#ifndef JOY_MESSAGE_TYPE_HPP
#define JOY_MESSAGE_TYPE_HPP

#include <cstdint>

namespace joysrv {
struct joy_message_type {
    std::uint32_t size;  // following message size

    enum struct joysrv_command : std::uint32_t {
        ADC_READ = 0,
        DAC_WRITE = 1,
        FPGA_READ = 2,
        FPGA_WRITE = 3,
        RESPONSE = 4,
    } message_type;
};

struct adc_read_type {
    std::uint8_t adc;
    std::uint8_t channel;
};

struct dac_write_type {
    std::uint8_t dac;
    std::uint8_t channel;
    std::uint8_t value;
};

struct fpga_read_type {
    std::uint8_t address;
};

struct fpga_write_type {
    std::uint8_t address;
    std::uint8_t value;
};

/////////////////////////////////////////////////////
// responses
struct response_type {
    std::uint32_t size;  // following message size

    enum struct joysrv_response : std::uint32_t {
        STATUS = 0,
        FPGA_READ = 1,
        ADC_READ = 1,
        FPGA_WRITE = 1,
    } message_type;
};

struct status_response_type {
    std::int32_t hv_mon;
    std::int32_t tube_current;
    std::int32_t cshvdc;
    std::int32_t cshv;
    std::int32_t csf;

    // These are not feedback values. They are just the last known values set.
    std::int32_t last_energy;
    std::int32_t last_intensity;
    std::int32_t last_xonoff;

    std::int32_t warming_up;

    std::int32_t batt_v;

    // temperatures
    std::int32_t temp_dio;  // temperature on dio board
    std::int32_t temp_xrg;  // temperature on x-ray generator
    std::int32_t temp_det;  // temperature on x-ray detector
    std::int32_t temp_act;  // temperature on the actuator control board

    std::uint32_t last_xtime;  // seconds elapsed since xray has been on
};

struct fpga_read_response_type {
    std::uint8_t status;  // non-zero - success, 0 - failure
    std::uint8_t response;
};

struct fpga_write_response_type {
    std::uint8_t status;  // non-zero - success, 0 - failure
};

struct adc_read_response_type {
    std::uint8_t response;
};

}  // namespace joysrv

#endif