#include "webcamera.hpp"

#include <fcntl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

WebCamera::WebCamera(std::string device_name) : dev_nm(device_name) {}

WebCamera::~WebCamera() {
  uninit_dev();
  close_dev();
}

void WebCamera::open_dev() {
  std::cout << "opening device " << dev_nm << std::endl;

  struct stat f_stat;
  if (stat(dev_nm.c_str(), &f_stat) == -1) {
    std::cout << "can't identify " << dev_nm << errno << strerror(errno)
              << std::endl;
    std::exit(EXIT_FAILURE);
  } else if (!S_ISCHR(f_stat.st_mode)) {
    std::cout << dev_nm << "is not a device" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  file_dscrptr = open(dev_nm.c_str(), O_RDWR);
  if (file_dscrptr == -1) {
    std::cout << "could not open " << dev_nm << std::endl;
    std::exit(EXIT_FAILURE);
  }

  std::cout << dev_nm << " opened successfully" << std::endl;
}

void WebCamera::init_dev() {
  std::cout << "initializing device" << std::endl;
  struct v4l2_capability cap;
  struct v4l2_format fmt;
  struct v4l2_streamparm streamparm;

  // get capabilities of device
  if (xioctl(VIDIOC_QUERYCAP, &cap) == -1) {
    std::cout << dev_nm << " is not a v4l2 device" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // check capabilities of device
  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    std::cout << dev_nm << " is not a video capture device" << std::endl;
    ;
    std::exit(EXIT_FAILURE);
  } else if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    std::cout << dev_nm << " does not support streaming" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // getting formats of the device
  struct v4l2_fmtdesc fmtdesc;
  CLEAR(fmtdesc);
  bool mjpeg_found = false;
  fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  while (xioctl(VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
    std::cout << "found format: "
              << reinterpret_cast<char *>(&fmtdesc.description) << std::endl;
    if (fmtdesc.pixelformat == V4L2_PIX_FMT_MJPEG) {
      mjpeg_found = true;
      break;
    }
    fmtdesc.index++;
  }

  if (!mjpeg_found) {
    std::cout << dev_nm << " does not appear to be a mjpeg device" << std::endl;
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
      auto fps = 1.0 * frmivalenum.discrete.denominator /
                 frmivalenum.discrete.numerator;
      if (frmivalenum.discrete.numerator != 1) {
        std::cout << "unsupported numerator in framerate" << std::endl;
        std::exit(EXIT_FAILURE);
      } else if ((fps < fps_denominator) && fps >= 15) {
        fps_denominator = static_cast<std::uint32_t>(fps);
      }
    }
    frmivalenum.index++;
  }

  std::cout << "found lowest supported framerate " << fps_denominator
            << std::endl;

  // setting framerate
  CLEAR(streamparm);
  streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  streamparm.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
  streamparm.parm.capture.timeperframe.numerator = 1;
  streamparm.parm.capture.timeperframe.denominator = fps_denominator;
  if (xioctl(VIDIOC_S_PARM, &streamparm) == -1) {
    std::cout << "error setting video stream params" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  std::cout << "framerate set to " << fps_denominator << std::endl;

  init_mmap();
}

int WebCamera::xioctl(unsigned long request, void *arg) {
  int res;

  do {
    res = ioctl(file_dscrptr, request, arg);
  } while (res == -1 && errno == EINTR);

  return res;
}

void WebCamera::init_mmap() {
  // request the buffers
  struct v4l2_requestbuffers requestbuffers;
  CLEAR(requestbuffers);
  requestbuffers.count = 1;
  requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  requestbuffers.memory = V4L2_MEMORY_MMAP;

  if (xioctl(VIDIOC_REQBUFS, &requestbuffers) == -1) {
    std::cout << "error: " << dev_nm << " does not support memory mapping"
              << std::endl;
    std::exit(EXIT_FAILURE);
    int joe;
  }

  // set and create the buffer
  struct v4l2_buffer buffer;
  CLEAR(buffer);
  buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;
  buffer.index = 0;

  if (xioctl(VIDIOC_QUERYBUF, &buffer) == -1) {
    std::cout << "error: could not query buffers" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  buf_size = buffer.length;
  auto res = mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                  file_dscrptr, buffer.m.offset);

  if (res == MAP_FAILED) {
    std::cout << "error: mapping failed" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  buf = static_cast<std::uint8_t *>(res);
}

void WebCamera::close_dev() {
  std::cout << "closing " << dev_nm << std::endl;
  if (::close(file_dscrptr) == -1) {
    std::cout << "error closing " << dev_nm << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

void WebCamera::uninit_dev() {
  std::cout << "uninitializing device" << std::endl;
  if (munmap(buf, buf_size) == -1) {
    std::cout << "error: munmap" << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

void WebCamera::stream_off() {
  // TODO
}

void WebCamera::stream_on() {
  // TODO
}

void WebCamera::read_frame() {
  if (streaming) {
    struct v4l2_buffer buffer;
    CLEAR(buffer);
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0;

    if (xioctl(VIDIOC_QBUF, &buffer) == -1) {
      std::cout << "query error" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    fd_set _fd_set;
    FD_ZERO(&_fd_set);
    FD_SET(file_dscrptr, &_fd_set);
    struct timeval tm_eval;
    CLEAR(tm_eval);
    tm_eval.tv_sec = 2;
    int r = select(file_dscrptr + 1, &_fd_set, NULL, NULL, &tm_eval);
    if (r == -1) {
      std::cout << "error: waiting on frame" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    if (xioctl(VIDIOC_DQBUF, &buffer) == -1) {
      std::cout << "error: retrieving frame" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    // TODO actually retrieve data and send it somewhere
  }
}