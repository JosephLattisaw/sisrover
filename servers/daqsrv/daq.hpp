#ifndef DAQ_HPP
#define DAQ_HPP

#include <boost/asio.hpp>
#include <cstdint>

#include "server.hpp"

class DAQ {
public:
    DAQ(std::uint16_t port, boost::asio::io_service &io_service);
    ~DAQ();

private:
    std::shared_ptr<Server> server;
};

#endif