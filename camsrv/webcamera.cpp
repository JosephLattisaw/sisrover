#include "webcamera.hpp"

#include <fcntl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#include "server.hpp"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

WebCamera::WebCamera(std::shared_ptr<Server> svr, boost::asio::io_service &controller_service,
                     boost::asio::io_service &io_service, std::string dn)
    : Camera(svr, controller_service, io_service, dn), timer(io_service) {
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Opening Device
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::cout << "opening device " << Camera::get_device_name() << std::endl;

    struct stat s;
    if (stat(Camera::get_device_name().c_str(), &s) == -1) {
        std::cout << "can't identify " << Camera::get_device_name() << errno << strerror(errno)
                  << std::endl;
        std::exit(EXIT_FAILURE);
    } else if (!S_ISCHR(s.st_mode)) {
        std::cout << Camera::get_device_name() << "is not a device" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    file_descriptor = open(Camera::get_device_name().c_str(), O_RDWR);
    if (file_descriptor == -1) {
        std::cout << "could not open " << Camera::get_device_name() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << Camera::get_device_name() << " opened successfully" << std::endl;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initializing Device
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::cout << "initializing device" << std::endl;
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_streamparm sp;

    // get capabilities of device
    if (xioctl(VIDIOC_QUERYCAP, &cap) == -1) {
        std::cout << Camera::get_device_name() << " is not a v4l2 device" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // check capabilities of device
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        std::cout << Camera::get_device_name() << " is not a video capture device" << std::endl;
        ;
        std::exit(EXIT_FAILURE);
    } else if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        std::cout << Camera::get_device_name() << " does not support streaming" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // getting formats of the device
    struct v4l2_fmtdesc fd;
    CLEAR(fd);
    bool mjpeg_found = false;
    fd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (xioctl(VIDIOC_ENUM_FMT, &fd) == 0) {
        std::cout << "found format: " << reinterpret_cast<char *>(&fd.description) << std::endl;
        if (fd.pixelformat == V4L2_PIX_FMT_MJPEG) {
            mjpeg_found = true;
            break;
        }
        fd.index++;
    }

    if (!mjpeg_found) {
        std::cout << Camera::get_device_name() << " does not appear to be a mjpeg device"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // setting format of device
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 320;
    fmt.fmt.pix.height = 240;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (xioctl(VIDIOC_S_FMT, &fmt) == -1) {
        std::cout << "error: could not set format" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // getting supported framerates
    struct v4l2_frmivalenum frmivalenum;
    CLEAR(frmivalenum);
    frmivalenum.height = 240;
    frmivalenum.width = 320;
    std::uint32_t fps_denominator = 30;
    frmivalenum.pixel_format = V4L2_PIX_FMT_MJPEG;

    while (xioctl(VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum) == 0) {
        if (frmivalenum.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
            auto fps = 1.0 * frmivalenum.discrete.denominator / frmivalenum.discrete.numerator;
            if (frmivalenum.discrete.numerator != 1) {
                std::cout << "unsupported numerator in framerate" << std::endl;
                std::exit(EXIT_FAILURE);
            } else if ((fps < fps_denominator) && fps >= 15) {
                fps_denominator = static_cast<std::uint32_t>(fps);
            }
        }
        frmivalenum.index++;
    }

    std::cout << "found lowest supported framerate " << fps_denominator << std::endl;

    // setting framerate
    CLEAR(sp);
    sp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sp.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
    sp.parm.capture.timeperframe.numerator = 1;
    sp.parm.capture.timeperframe.denominator = fps_denominator;
    if (xioctl(VIDIOC_S_PARM, &sp) == -1) {
        std::cout << "error setting video stream params" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << "framerate set to " << fps_denominator << std::endl;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initializing Memory Map
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // request the buffers
    struct v4l2_requestbuffers rb;
    CLEAR(rb);
    rb.count = 1;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;

    if (xioctl(VIDIOC_REQBUFS, &rb) == -1) {
        std::cout << "error: " << Camera::get_device_name() << " does not support memory mapping"
                  << std::endl;
        std::exit(EXIT_FAILURE);
        int joe;
    }

    // set and create the buffer
    struct v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (xioctl(VIDIOC_QUERYBUF, &buf) == -1) {
        std::cout << "error: could not query buffers" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    buffer_size = buf.length;
    auto res =
        mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, buf.m.offset);

    if (res == MAP_FAILED) {
        std::cout << "error: mapping failed" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    buffer = static_cast<std::uint8_t *>(res);
}

WebCamera::~WebCamera() {
    // uninitializing device
    std::cout << "uninitializing device" << std::endl;
    if (munmap(buffer, buffer_size) == -1) {
        std::cout << "error: munmap" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // closing device
    std::cout << "closing " << Camera::get_device_name() << std::endl;
    if (::close(file_descriptor) == -1) {
        std::cout << "error closing " << Camera::get_device_name() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

int WebCamera::xioctl(unsigned long request, void *arg) {
    int res;

    do {
        res = ioctl(file_descriptor, request, arg);
    } while (res == -1 && errno == EINTR);

    return res;
}

void WebCamera::read_frame() {
    timer.async_wait([&](boost::system::error_code error) {
        if (Camera::is_streaming() && !error) {
            struct v4l2_buffer buf;
            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = 0;

            if (xioctl(VIDIOC_QBUF, &buf) == -1) {
                std::cout << "query error" << std::endl;
                std::exit(EXIT_FAILURE);
            }

            fd_set fs;
            FD_ZERO(&fs);
            FD_SET(file_descriptor, &fs);
            struct timeval tv;
            CLEAR(tv);
            tv.tv_sec = 2;
            int r = select(file_descriptor + 1, &fs, NULL, NULL, &tv);
            if (r == -1) {
                std::cout << "error: waiting on frame" << std::endl;
                std::exit(EXIT_FAILURE);
            }

            if (xioctl(VIDIOC_DQBUF, &buf) == -1) {
                std::cout << "error: retrieving frame" << std::endl;
                std::exit(EXIT_FAILURE);
            }

            std::vector<std::uint8_t> sf;
            sf.resize(static_cast<int>(buf.bytesused));
            memcpy(sf.data(), buffer, buf.bytesused);
            controller_service.post(std::bind(&Server::send_frame, server, sf));

            read_frame();  // recursively read webcam data (but in event loop)
        }
    });
}

void WebCamera::send_stream_request(unsigned long request) {
    // telling camera to turn on
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(request, &type) == -1) {
        std::cout << "error: stopping camera stream" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void WebCamera::set_stream(bool on) {
    if (on) {
        std::cout << "wc: starting webcamera stream" << std::endl;

        send_stream_request(VIDIOC_STREAMON);

        Camera::set_stream(true);  // start streaming to server
        read_frame();              // start trying to read frames from device
    } else {
        std::cout << "wc: stopping webcamera stream" << std::endl;

        send_stream_request(VIDIOC_STREAMOFF);

        // resetting stream parameters
        Camera::set_stream(false);  // stop streaming to server
        timer.cancel();             // stop trying to read frames from device

        std::cout << "wc: camera stream stopped" << std::endl;
    }
}