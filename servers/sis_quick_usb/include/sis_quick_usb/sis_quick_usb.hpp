#ifndef SIS_QUICK_USB_HPP
#define SIS_QUICK_USB_HPP

namespace squsb {
class squsb {
public:
    squsb();

    bool connect_to_qusb(const char begin_serial_number, signed long timeout);

private:
    bool connected = false;

    char current_begin_serial_number;
    signed long current_quick_usb_timeout;
};
}  // namespace squsb

#endif