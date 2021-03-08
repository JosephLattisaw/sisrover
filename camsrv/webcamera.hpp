#ifndef webcamera__HPP
#define webcamera__HPP

#include <string>

class WebCamera {
public:
    WebCamera(std::string device_name);
    ~WebCamera();

    void stream_off();
    void stream_on();

private:
    // device logic
    void close_dev();  // closes device
    void init_dev();   // initializes device
    void init_mmap();
    void open_dev();    // opens device
    void uninit_dev();  // uninitialize device
    int xioctl(unsigned long request, void *arg);
    void read_frame();

    std::uint8_t *buf;
    size_t buf_size;
    std::string dev_nm;
    int file_dscrptr;
    bool streaming = false;
};

#endif