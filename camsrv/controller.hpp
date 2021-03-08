#ifndef controller__HPP
#define controller__HPP

#include <string>

class Controller {
 public:
  Controller(std::string device_name, std::uint16_t port, std::string url);
};

#endif