#ifndef controller__HPP
#define controller__HPP

#include <boost/asio.hpp>
#include <string>

#include "server.hpp"

class Controller {
public:
    Controller(std::string device_name, std::uint16_t port, std::string url,
               boost::asio::io_service &io_service);
    ~Controller();

private:
    boost::asio::io_service &io_service;
    std::unique_ptr<Server> server;
};

#endif