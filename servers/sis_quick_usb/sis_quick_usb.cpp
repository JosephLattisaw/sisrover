#include "sis_quick_usb.hpp"

// standard includes
#include <iostream>
#include <vector>

namespace {
const unsigned char POWERDAC = 0x03;
const unsigned char DAC_ADDRESS[3] = {0x10, 0x11, 0x12};
const unsigned char ADC_ADDRESS[3] = {0x48, 0x49, 0x4a};

void print_dev_null_error() { std::cerr << "squsb: failed: device handle is null" << std::endl; }

void print_last_error_message() {
    QULONG ec;
    QuickUsbGetLastError(&ec);
    std::cerr << "squsb: last error code: " << ec << std::endl;
}
}  // namespace

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

    for (const auto& i : dev_idx) {
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

std::tuple<bool, unsigned char> squsb::squsb::read_fpga(QWORD address) {
    unsigned char dat[1];
    QWORD l = sizeof(dat);
    bool res = read_quickusb_command(address, &dat[0], &l);
    return std::make_tuple(res, dat[0]);
}

std::tuple<bool, QWORD> squsb::squsb::read_quickusb_setting(QWORD address) {
    QWORD stg;
    bool res = false;

    std::cout << "squsb: reading quickusb setting " << static_cast<int>(address) << std::endl;

    if (dev_handle == nullptr)
        print_dev_null_error();
    else if (!QuickUsbReadSetting(dev_handle, address, &stg)) {
        std::cerr << "squsb: failed to read QuickUsb read setting" << std::endl;
        print_last_error_message();
    } else
        res = true;

    return std::make_tuple(res, stg);
}

bool squsb::squsb::write_fpga(QWORD address, char value) {
    unsigned char dat[1];
    dat[1] = value;
    QWORD l = sizeof(dat);
    write_quickusb_command(address, &dat[0], l);
    return write_quickusb_command(address, &dat[0], l);  // writing a second time just in case
}

bool squsb::squsb::read_quickusb_command(QWORD address, unsigned char* destination, QWORD* length) {
    std::cout << "squsb: reading quickusb command at address: " << static_cast<int>(address)
              << std::endl;
    if (!QuickUsbReadCommand(dev_handle, address, destination, length)) {
        std::cerr << "squsb: failed to read command from QuickUSB" << std::endl;
        return false;
    }

    return true;
}

bool squsb::squsb::write_quickusb_command(QWORD address, unsigned char* data, QWORD length) {
    std::cout << "squsb: writing quickusb command to address " << static_cast<int>(address)
              << std::endl;

    if (dev_handle == nullptr)
        print_dev_null_error();
    else if (!QuickUsbWriteCommand(dev_handle, address, data, length)) {
        std::cerr << "squsb: failed to write quickusb command" << std::endl;
        print_last_error_message();
    } else
        return true;

    return false;
}

std::tuple<bool, unsigned char> squsb::squsb::read_adc(QWORD address, unsigned char* data) {
    if (dev_handle == nullptr) {
        print_dev_null_error();
        return std::make_tuple(false, 0);
    }

    // set the channel for the ADC
    auto res = QuickUsbWriteI2C(dev_handle, address, data, 1);

    // read the value
    unsigned short l = 1;
    unsigned char val;
    res = QuickUsbReadI2C(dev_handle, address, &val, &l);
    return std::make_tuple(static_cast<bool>(res), val);
}

bool squsb::squsb::read_data(unsigned char* data, unsigned long* length) {
    if (dev_handle != nullptr && QuickUsbReadData(dev_handle, data, length))
        return true;
    else if (dev_handle == nullptr)
        print_dev_null_error();

    std::cerr << "squsb: failed to read data from QuickUsb" << std::endl;
    print_last_error_message();
    return false;
}

bool squsb::squsb::set_DAC(unsigned char dac, unsigned char channel, unsigned char value) {
    std::cout << "squsb: setting DAC " << static_cast<int>(dac) << " channel "
              << static_cast<int>(channel) << " value " << static_cast<int>(value) << std::endl;

    // get command and channel value
    unsigned char cc = (POWERDAC << 4) | (channel & 0x0f);

    unsigned char dat[] = {cc, value, 0};

    if (dev_handle == nullptr)
        print_dev_null_error();
    else if (!QuickUsbWriteI2C(dev_handle, DAC_ADDRESS[dac], dat, sizeof(dat))) {
        std::cerr << "squsb: failed to write dac value" << std::endl;
        print_last_error_message();
    } else
        return true;

    return false;
}

bool squsb::squsb::set_port_direction(int port, int direction) {
    std::cout << "squsb: setting port " << port << " to direction " << direction << std::endl;

    if (dev_handle == nullptr)
        print_dev_null_error();
    else if (!QuickUsbWritePortDir(dev_handle, port, direction)) {
        std::cerr << "squsb: failed to write quickusb port direction" << std::endl;
        print_last_error_message();
    } else
        return true;

    return false;
}

bool squsb::squsb::write_port(unsigned short address, unsigned char* data, unsigned short length) {
    std::cout << "writing port " << static_cast<int>(address) << std::endl;

    if (dev_handle == nullptr)
        print_dev_null_error();
    else if (!QuickUsbWritePort(dev_handle, address, data, length)) {
        std::cerr << "squsb: failed to write port value" << std::endl;
        print_last_error_message();
    } else
        return true;

    return false;
}

bool squsb::squsb::write_quickusb_setting(QWORD address, QWORD setting) {
    std::cout << "squsb: writing quickusb setting " << static_cast<int>(setting) << " to address "
              << static_cast<int>(address) << std::endl;

    if (dev_handle == nullptr)
        print_dev_null_error();
    else if (!QuickUsbWriteSetting(dev_handle, address, setting)) {
        std::cerr << "squsb: failed to write quickusb setting" << std::endl;
        print_last_error_message();
    } else
        return true;

    return false;
}