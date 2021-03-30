#include "controller.hpp"

#include <iostream>

Controller::Controller(bool tst_mo, char number_of_asics, unsigned int ms_buff,
                       unsigned int sample_time, unsigned int dv, unsigned int ser_no,
                       unsigned int buffer_size, unsigned int timeout, unsigned int read_multiple)
    : test_mode(tst_mo), quickusb_timeout(timeout), daq_version(dv) {
    serial_number = std::to_string(ser_no).c_str()[0];

    // TODO open and read data file

    quickusb = std::make_shared<squsb::squsb>();
    std::cout << "controller: using sis_quick_usb version number: "
              << quickusb->get_version_number() << std::endl;
}

Controller::~Controller() {}

void Controller::connect_to_daq() {
    quickusb->connect_to_qusb(serial_number, quickusb_timeout);
    setup_daq();
}

void Controller::setup_daq() {
    set_default_quickusb_settings();

    switch (daq_version) {
        case DAQ_VERSIONS::VERSION_1:
            std::cout << "controller: using daq version 1" << std::endl;
            initialize_daq_version_1();
            break;
        case DAQ_VERSIONS::VERSION_2:
            std::cout << "controller: using daq version 2" << std::endl;
            initialize_daq_version_2();
            break;
        default:
            std::cerr << "controller: unaccounted for daq version" << std::endl;
            std::exit(EXIT_FAILURE);
    }

    connected = true;

    std::cout << "controller: starting to scan" << std::endl;
    start_scanning();
}

void Controller::set_default_quickusb_settings() {
    // TODO
    std::cout << "controller: setting default quickusb settings" << std::endl;

    // setting settings for first address
    const auto ADR_1 = 1;
    auto t_1 = quickusb->read_quickusb_setting(ADR_1);
    verify_truth(std::get<0>(t_1), __PRETTY_FUNCTION__, "failed to read quickusb setting 1");
}

void Controller::initialize_daq_version_1() {
    // TODO
}

void Controller::initialize_daq_version_2() {
    // TODO
}

void Controller::start_scanning() {
    // TODO
}

void Controller::verify_truth(bool truth, std::string function_name, std::string error) {
    if (!truth) {
        std::cerr << "controller: failure " << function_name << ", error: " << error << std::endl;
    }
}