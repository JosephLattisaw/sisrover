#include "daq.hpp"

DAQ::DAQ(std::uint16_t port, daqsrv::controller_options_type &controller_options,
         boost::asio::io_service &io_service)
    : io_service(io_service) {
    server = std::make_shared<Server>(io_service, port, [&](daqsrv::daq_settings_type daq_stgs) {
        daq_settings = daq_stgs;
        daq_settings_updated = true;
        if (controller)
            controller_service.post(
                std::bind(&Controller::update_settings, std::ref(controller), daq_settings));
    });
    controller_thread = std::thread(std::bind(&DAQ::worker_thread, this, controller_options));
}

DAQ::~DAQ() {
    controller_thread.join();
    controller_service.stop();
}

void DAQ::worker_thread(daqsrv::controller_options_type &controller_options) {
    controller = std::make_unique<Controller>(controller_options, controller_service,
                                              [&](std::vector<std::uint8_t> data) {

                                              });

    io_service.post([&]() {
        if (daq_settings_updated) controller->update_settings(daq_settings);
    });

    // This stops the thread from exiting just because we don't have any tasks that currently
    // need completing
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(
        controller_service.get_executor());
    controller_service.run();
}