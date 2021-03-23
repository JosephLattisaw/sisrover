#include "camera.hpp"

#include "server.hpp"

Camera::Camera(std::shared_ptr<Server> svr, boost::asio::io_service &controller_service,
               boost::asio::io_service &io_service, std::string dev_name)
    : server(svr),
      controller_service(controller_service),
      io_service(io_service),
      device_name(dev_name) {}

Camera::~Camera() {}

void Camera::set_stream(bool on) { streaming = on; }

bool Camera::is_streaming() const { return streaming; }

std::string Camera::get_device_name() const { return device_name; }