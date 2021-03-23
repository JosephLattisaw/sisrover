#include "controller.hpp"

#include <iostream>

Controller::Controller(std::string device_name, std::uint16_t port, std::string url,
                       boost::asio::io_service &io_service)
    : io_service(io_service), timer(io_service) {
    server = std::make_shared<Server>(io_service, port, [&](bool stream) {
        if (camera) camera_service.post(std::bind(&Camera::set_stream, std::ref(camera), stream));
    });
    camera_thread = std::thread(std::bind(&Controller::worker_thread, this, device_name, url));
}

Controller::~Controller() {
    camera_thread.join();
    camera_service.stop();
}

void Controller::worker_thread(std::string device_name, std::string url) {
    bool use_url_node = !url.empty();

    if (use_url_node)
        camera = std::make_unique<IPCamera>(server, io_service, camera_service, url);
    else
        camera = std::make_unique<WebCamera>(server, io_service, camera_service, device_name);

    // Avoids race condition. What if somehow the server has already been connected and sent a
    // stream on/off command. Our camera object in a different thread may not have been
    // substantiated in time. Easiest solution is to just request the current status upon
    // initialization
    io_service.post([&]() { server->request_stream_status_update(); });

    // This stops the thread from exiting just because we don't have any tasks that currently
    // need completing
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(
        camera_service.get_executor());
    camera_service.run();
}