#ifndef DAQ_MESSAGE_TYPE_HPP
#define DAQ_MESSAGE_TYPE_HPP

#include <cstdint>
#include <vector>

namespace daqsrv {
struct daq_message_type {
    std::uint32_t size;  // following message size

    enum struct daqsrv_command : std::uint32_t {
        KEEP_ALIVE = 0,
        START_OFFLINE = 1,
        START_ONLINE = 2,
        START_SCAN = 3,
        STOP_DATA = 4,
        SCAN_DATA = 5,
        SCAN_BUFFER_EXCEEDED = 6,
        SCAN_TIME_EXCEEDED = 7,
        DAQ_SETTINGS = 8,
    } message_type;
};

struct daq_settings_type {
    std::uint32_t buffer_size;
    std::uint32_t ms_buff;
    std::uint32_t number_of_asics;
    std::uint32_t quickusb_timeout;
    std::uint16_t read_multiple;
    std::uint16_t timing;
};
}  // namespace daqsrv

#endif