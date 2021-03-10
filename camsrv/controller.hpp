#ifndef controller__HPP
#define controller__HPP

// standard includes
#include <string>
#include <thread>

// boost includes
#include <boost/asio.hpp>

#include "ipcamera.hpp"
#include "server.hpp"
#include "webcamera.hpp"

// TODO a lot of this needs to be reworked

class Controller {
public:
    Controller(std::string device_name, std::uint16_t port, std::string url,
               boost::asio::io_service &io_service);
    ~Controller();

private:
    void start_threads(
        std::string device_name,
        std::string url);  // TODO probably doesn't need to be a function, do lambda instead

    // asio services
    boost::asio::io_service camera_service;
    boost::asio::io_service &io_service;

    // cameras
    // TODO see if we can make a base class out of these both and only have one unique ptr :)
    std::unique_ptr<IPCamera> ip_camera;
    std::unique_ptr<WebCamera> web_camera;
    std::thread camera_thread;

    std::shared_ptr<Server> server;

    boost::asio::steady_timer timer;  // keep-alive timer
};

#endif