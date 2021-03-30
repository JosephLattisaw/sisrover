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
    void initialize_daq_version_1();
    void initialize_daq_version_2();

    void connect_to_daq();
    void setup_daq();
    void set_default_quickusb_settings();
    void start_scanning();

    void verify_truth(bool truth, std::string function_name, std::string error);

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

    unsigned int daq_version;  // version of daqsrv api

    enum DAQ_VERSIONS {
        VERSION_1 = 1,
        VERSION_2 = 2,
    };
};

#endif