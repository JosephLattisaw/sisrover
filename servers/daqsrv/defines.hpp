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

struct controller_options_type {
    bool test_mode;                         // port number to be used for our tcp/ip server
    std::uint16_t number_of_asics = 16;     // number of asics detector arm is using
    std::uint16_t ms_buff = 119;            // upper 7 bits of ADDCNT
    std::uint16_t sample_time = 4;          // sample time of PAREG register
    std::uint16_t daq_version;              // version of daq we are using
    std::uint16_t serial_number;            // serial number of quickusb board
    std::uint32_t buffer_size = 122880;     // size of buffer to read data off quickusb board
    std::uint16_t read_multiple = 1;        // read multiples of buffer size
    std::uint32_t quickusb_timeout = 1000;  // timeout for quickusb requests
};
}  // namespace daqsrv

#endif