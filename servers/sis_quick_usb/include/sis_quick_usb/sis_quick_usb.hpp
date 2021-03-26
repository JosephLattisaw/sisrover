#ifndef SIS_QUICK_USB_HPP
#define SIS_QUICK_USB_HPP

#include "QuickUSB.h"

namespace squsb {
class squsb {
public:
    squsb();

    // connecting to quickusb
    bool connect_to_qusb(const char begin_serial_number, QLONG timeout);
    bool disconnect_from_qusb();

private:
    void print_last_error_message();

    char current_begin_serial_number;
    QLONG current_quick_usb_timeout;

    QHANDLE dev_handle = nullptr;
};
}  // namespace squsb

#endif