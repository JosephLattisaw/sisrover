#ifndef CAM_CLIENT__HPP
#define CAM_CLIENT__HPP

#include <boost/asio.hpp>

#include "camsrv_msg.hpp"

class Cam_Client {
public:
    Cam_Client(boost::asio::io_service &io_service);

private:
    void reset();
    void reset_buffers();

    void start_read();
    void start_async_connect();

    void start_keepalive();

    boost::asio::streambuf header_buffer;
    boost::asio::streambuf data_buffer;

    boost::asio::ip::tcp::endpoint endpoint;
    boost::asio::io_service &io_service;
    boost::asio::ip::tcp::socket socket;
    boost::asio::steady_timer timer;
};

#endif