#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <memory>
#include <sis_quick_usb/sis_quick_usb.hpp>

class Controller {
public:
    Controller(bool test_mode, char number_of_asics, unsigned int ms_buff, unsigned int sample_time,
               unsigned int daq_version, unsigned int serial_number, unsigned int buffer_size,
               unsigned int timeout, unsigned int read_multiple);
    ~Controller();

private:
    // deciding to initializa which daq version api
    void initialize_daq_version_1();
    void initialize_daq_version_2();

    // main daq function
    void connect_to_daq();
    void setup_daq();
    void set_default_quickusb_settings();
    void start_scanning();

    // utility functions
    char get_pareg(unsigned int sample_time);
    void verify_truth(bool truth, std::string function_name, std::string error);
    unsigned char three_bits(unsigned long from, int first_bit);

    // old daq api
    enum COMMAND {
        ARRAY_MASK20 = 0x0d,
        ARRAY_MASK53 = 0x0e,
        ARRAY_MASK86 = 0x0f,
        ARRAY_MASK119 = 0x10,
        TA = 0x11,
        ADDCNT = 0x13,
        ASICNUM = 0x14,
        SOFT_RESET = 0x15,
    };

    // daq version 1 commands
    void daqv1_start_commanding();
    void daqv1_send_command(COMMAND command, unsigned char arg);
    void daqv1_set_array_a_timing(int milliseconds);
    void daqv1_set_array_mask(unsigned int mask);
    void daqv1_set_buffer_size(int size);
    void daqv1_set_number_of_asics(int number_of_asics);
    void daqv1_reset();

    // register values
    char ms_buff;  // value written to msbuff register
    // number of asics we are using (this = 1 equals value written to
    // register)
    char number_of_asics;
    unsigned int sample_time;  // sample time used to find pareg register value
    char serial_number;        // first serial number in quickusb

    // quickusb
    bool connected = false;  // connected to quickusb board
    std::shared_ptr<squsb::squsb> quickusb;
    unsigned int quickusb_timeout;

    // test mode
    bool test_mode = false;
    bool test_server = false;

    // fpga addresses
    const QWORD ASICS_NUMBER_ADDRESS = 0x8;
    const QWORD CONFIGURATION_ADDRESS = 0x0b;
    const QWORD MSBUFF_ADDRESS = 0x9;
    const QWORD PAREG_ADDRESS = 0xa;
    const QWORD START_RESET_ADDRESS = 0x0f;

    // default values
    const char DEFAULT_BEGIN_SERIAL_NUMBER = '1';
    const char DEFAULT_CONFIGURATION_VALUE = 0x0;
    const QLONG DEFAULT_QUICK_USB_TIMEOUT = 1000;
    const char START_RESET_START_DATA_ACQ_VALUE = 2;
    const char START_RESET_RESET_FPGA_VALUE = 1;

    // miscellaneous values
    unsigned int buffer_size;
    unsigned int read_multiple;
    unsigned int daq_version;  // version of daqsrv api

    enum DAQ_VERSIONS {
        VERSION_1 = 1,
        VERSION_2 = 2,
    };

    // utility mi/mo functions
    int mi_data_len;
    unsigned char mo_command_data[64];
};

#endif