#include "daq.hpp"

DAQ::DAQ(std::uint16_t port, boost::asio::io_service &io_service) {
    server = std::make_shared<Server>(io_service, port);
}

DAQ::~DAQ() {}