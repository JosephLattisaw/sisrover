#include "sis_quick_usb.hpp"

#include <iostream>
#include <vector>

squsb::squsb::squsb() {}

bool squsb::squsb::connect_to_qusb(const char begin_serial_number, signed long timeout) {
    std::cout << "squsb: attempting to connect to quickusb" << std::endl;
    current_begin_serial_number = begin_serial_number;
    current_quick_usb_timeout = timeout;

    char dev_name[100];
    std::vector<int> dev_idx;

}
