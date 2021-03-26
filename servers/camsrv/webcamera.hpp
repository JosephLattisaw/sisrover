#ifndef webcamera__HPP
#define webcamera__HPP

// standard includes
#include <string>

// boost includes
#include <boost/asio.hpp>

#include "camera.hpp"

class Server;  // forward declaration

class WebCamera : public Camera {
public:
    WebCamera(std::shared_ptr<Server> server, boost::asio::io_service &controller_service,
              boost::asio::io_service &io_service, std::string device_name);
    ~WebCamera();

    void set_stream(bool on);  // lets you turn stream on/off

private:
    void read_frame();
    void send_stream_request(unsigned long request);
    int xioctl(unsigned long request, void *arg);  // utility helper

    // buffer
    std::uint8_t *buffer;
    size_t buffer_size;

    int file_descriptor;
    boost::asio::steady_timer timer;
};

#endif