#include "sis_quick_usb.hpp"

// standard includes
#include <iostream>
#include <vector>

squsb::squsb::squsb() {}

bool squsb::squsb::connect_to_qusb(const char begin_serial_number, QLONG timeout) {
    std::cout << "squsb: attempting to connect to quickusb" << std::endl;
    current_begin_serial_number = begin_serial_number;
    current_quick_usb_timeout = timeout;

    char dev_name[100];
    std::vector<int> dev_idx;

    // should return non zero number on success, zero is failure
    if (QuickUsbFindModules(dev_name, sizeof(dev_name))) {
        for (auto i = 0; i < 15;) {  // TODO this shouldn't be hardcoded like this and why 15?
            auto len = strlen(&dev_name[i]);
            if (len == 0)
                break;
            else {
                dev_idx.push_back(i);
                std::cout << "squsb: Found: " << std::string(&dev_name[i]).c_str() << std::endl;
            }
        }
    } else {
        std::cerr << "squsb: failed to find any quick usb modules" << std::endl;
        print_last_error_message();
        return false;
    }

    for (const auto &i : dev_idx) {
        auto dn = std::string(&dev_name[i]);
        if (QuickUsbOpen(&dev_handle, &dev_name[i])) {
            std::cout << "squsb: opened quickusb device: " << dn << std::endl;

            // setting default timeout of QuickUsb Module
            if (QuickUsbSetTimeout(dev_handle, current_quick_usb_timeout)) {
                std::cout << "squsb: set quickusb timeout to " << current_quick_usb_timeout
                          << std::endl;
            } else {
                std::cerr << "squsb: failed to set quick usb timeout" << std::endl;
                print_last_error_message();
                return false;
            }

            // getting serial number from quickusb
            char serial[128];
            if (QuickUsbGetStringDescriptor(dev_handle, QUICKUSB_SERIAL, serial, sizeof(serial))) {
                std::cout << "found serial number: " << dn << std::endl;
                if (serial[0] == current_begin_serial_number) {
                    std::cout << "squsb: DAQ QuickUSB confirmed by serial number" << std::endl;
                    return true;
                }
            } else {
                std::cerr << "squsb: failed to find serial number of quickusb device: " << dn
                          << std::endl;
                print_last_error_message();
                return false;
            }

            // if we make it to here this is not the usb we want so we close it
            if (!QuickUsbClose(dev_handle)) {
                std::cerr << "squsb: failed to close: " << dn << std::endl;
                print_last_error_message();
                return false;
            } else
                std::cout << "squsb: closed quickusb device: " << dn << std::endl;
        } else {
            std::cerr << "squsb: failed to open quickusb device: " << dn << std::endl;
            print_last_error_message();
        }
    }

    std::cerr << "squsb: failed to connect to QuickUSB Device" << std::endl;
    return false;
}

bool squsb::squsb::disconnect_from_qusb() {
    // nonzero on success
    if (!QuickUsbClose(dev_handle)) {
        std::cerr << "squsb: failed to close QuickUSB device" << std::endl;
        print_last_error_message();
        return false;
    }

    return true;
}

void squsb::squsb::print_last_error_message() {
    // TODO
}