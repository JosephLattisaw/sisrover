#include <boost/asio.hpp>
#include <iostream>

#include "cam_client.hpp"

extern "C" {
void say_hello() { std::cout << "Hello, from cam_client_backend!\n"; }
}

int main(int argc, char **argv) {
    say_hello();
    boost::asio::io_service io_service;
    auto cam_client = std::make_unique<Cam_Client>(io_service);
    io_service.run();
    return 0;
}
