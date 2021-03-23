#ifndef camera__HPP
#define camera__HPP

// boost includes
#include <boost/asio.hpp>

class Server;  // forward declaration
class Camera {
public:
    Camera(std::shared_ptr<Server> server, boost::asio::io_service &controller_service,
           boost::asio::io_service &io_service, std::string device_name);
    ~Camera();

    virtual void set_stream(bool on);

protected:
    std::string get_device_name() const;
    bool is_streaming() const;

    // services
    boost::asio::io_service &controller_service;  // controller object threads service
    boost::asio::io_service &io_service;          // service used by IPCamera object

    std::shared_ptr<Server> server;  // reference to server to be able to send data

private:
    std::string device_name;
    bool streaming = false;
};

#endif