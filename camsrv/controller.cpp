#include "controller.hpp"

#include <iostream>

Controller::Controller(std::string device_name, std::uint16_t port, std::string url,
                       boost::asio::io_service &io_service)
    : io_service(io_service), server(std::make_unique<Server>(io_service, port)) {
    bool use_url_node = !url.empty();

    if (use_url_node) {
        // TODO
    } else {
        // TODO
    }
}

Controller::~Controller() { std::cout << "controller destroyed" << std::endl; }