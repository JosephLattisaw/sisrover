#ifndef DAQSRV_DEFINES_HPP
#define DAQSRV_DEFINES_HPP

#include <array>
#include <list>
#include <map>

namespace daqsrv {
const std::string DAQSRV_APPLICATION_NAME = "daqsrv";  // name of application
const std::string DAQSRV_VERSION_NUMBER = DAQSRV_APPLICATION_NAME + " 4.0.0";  // version number
const std::string DAQSRV_APPLICATION_DESCRIPTION =
    "data acquisition server which feeds image data from remote cpu";  // application description

// number constraints for port options
const std::uint16_t MINIMUM_PORT_NUMBER = 1;
const std::uint16_t MAXIMUM_PORT_NUMBER = 65535;

const std::list<std::uint16_t> VALID_SAMPLE_TIMES = {4, 8, 16, 32, 64};
const std::map<std::uint16_t, char> SAMPLE_TIME_MAP = {
    {4, 0b0000}, {8, 0b001}, {16, 0b010}, {32, 0b011}, {64, 0b100}};

const std::list<std::uint16_t> VALID_DAQ_VERSIONS = {1, 2};
}  // namespace daqsrv

#endif