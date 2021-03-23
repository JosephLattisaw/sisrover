#ifndef controller__HPP
#define controller__HPP

// standard includes
#include <string>
#include <thread>

// boost includes
#include <boost/asio.hpp>

#include "camera.hpp"
#include "ipcamera.hpp"
#include "server.hpp"
#include "webcamera.hpp"

class Controller {
public:
    Controller(std::string device_name, std::uint16_t port, std::string url,
               boost::asio::io_service &io_service);
    ~Controller();

private:
    void worker_thread(std::string device_name, std::string url);

    // asio services
    boost::asio::io_service camera_service;
    boost::asio::io_service &io_service;

    // camera
    std::unique_ptr<Camera> camera;
    std::thread camera_thread;

    std::shared_ptr<Server> server;

    boost::asio::steady_timer timer;  // keep-alive timer
};

#endif