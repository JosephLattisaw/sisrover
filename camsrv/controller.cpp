#include "controller.hpp"

#include <iostream>

Controller::Controller(std::string device_name, std::uint16_t port, std::string url,
                       boost::asio::io_service &io_service)
    : io_service(io_service),
      server(std::make_shared<Server>(io_service, port)),
      timer(io_service) {
    bool use_url_node = !url.empty();

    camera_thread = std::thread(std::bind(&Controller::start_threads, this, device_name, url));
}

Controller::~Controller() { std::cout << "controller destroyed" << std::endl; }

void Controller::start_threads(std::string device_name, std::string url) {
    bool use_url_node = !url.empty();

    if (use_url_node)
        ip_camera = std::make_unique<IPCamera>(server, io_service, camera_service, url);
    else
        web_camera = std::make_unique<WebCamera>(device_name);

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(
        camera_service.get_executor());  // TODO probably don't want this without exit
    camera_service.run();
}