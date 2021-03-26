#ifndef camsrv_DEFINES__HPP
#define camsrv_DEFINES__HPP

#include <string>

namespace camsrv {
const std::string CAMSRV_APPLICATION_NAME = "camsrv";
const std::string CAMSRV_VERSION_NUMBER = CAMSRV_APPLICATION_NAME + " 4.0.0";  // version number
const std::string CAMSRV_APPLICATION_DESCRIPTION =
    "camera server which feeds webcam images from remote cpu";  // application description
}  // namespace camsrv

#endif