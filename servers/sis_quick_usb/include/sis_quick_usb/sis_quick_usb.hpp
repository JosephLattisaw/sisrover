#ifndef SIS_QUICK_USB_HPP
#define SIS_QUICK_USB_HPP

#include <tuple>
#include <vector>

#include "QuickUSB.h"

namespace squsb {
class squsb {
public:
    squsb();

    // connecting to quickusb
    bool connect_to_qusb(const char begin_serial_number, QLONG timeout);
    bool disconnect_from_qusb();

    // fpga
    std::tuple<bool, unsigned char> read_fpga(QWORD address);
    bool write_fpga(QWORD address, char value);

    // quickusb commands
    bool read_quickusb_command(QWORD address, unsigned char* destination, QWORD* length);
    bool write_quickusb_command(QWORD address, unsigned char* data, QWORD length);

    // other quickusb commands
    std::tuple<bool, unsigned char> read_adc(QWORD address, unsigned char* data);
    bool read_data(unsigned char* data, unsigned long* length);
    bool set_DAC(unsigned char dac, unsigned char channel, unsigned char value);
    bool set_port_direction(int port, int direction);
    bool write_port(unsigned short address, unsigned char* data, unsigned short length);

    // other
    std::string get_version_number() const { return VERSION_NUMBER; }

private:
    // print helpers
    void print_dev_null_error();
    void print_last_error_message();

    unsigned char command_and_chan_val(unsigned char cmd, unsigned chan);

    char current_begin_serial_number;
    QLONG current_quick_usb_timeout;

    QHANDLE dev_handle = nullptr;

    const std::string VERSION_NUMBER = "1.2.1";
};
}  // namespace squsb

#endif