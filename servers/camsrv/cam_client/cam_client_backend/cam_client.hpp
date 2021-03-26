#ifndef CAM_CLIENT__HPP
#define CAM_CLIENT__HPP

#include <boost/asio.hpp>

#include "camsrv_msg.hpp"

class Cam_Client {
    using Connection_Callback = std::function<void(bool)>;
    using Image_Callback = std::function<void(std::vector<std::uint8_t>)>;

public:
    Cam_Client(boost::asio::io_service &io_service, Connection_Callback conn_cb,
               Image_Callback image_cb);

    void set_connection(bool status);

private:
    void reset();
    void reset_buffers();

    void start_read();
    void start_async_connect();

    void start_keepalive();
    void updated_connection_status(bool status);

    boost::asio::streambuf header_buffer;
    boost::asio::streambuf data_buffer;

    boost::asio::ip::tcp::endpoint endpoint;
    boost::asio::io_service &io_service;
    boost::asio::ip::tcp::socket socket;
    boost::asio::steady_timer timer;

    Connection_Callback connection_callback;
    Image_Callback image_callback;

    bool connection_status = false;
};

#endif