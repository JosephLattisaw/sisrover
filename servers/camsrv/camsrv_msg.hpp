#ifndef CAMSRV_MSG
#define CAMSRV_MSG

#include <cstdint>
#include <vector>

namespace camsrv {
struct camsrv_message {
    std::uint32_t size = 0;    // size of image if there is one
    std::uint16_t width = 0;   // width of image
    std::uint16_t height = 0;  // height of image

    enum struct camsrv_command : std::uint32_t {
        IMAGE = 0,       // what the gui client receives
        KEEP_ALIVE = 1,  // keep alive message
        STREAM_ON = 2,   // enables the stream
        STREAM_OFF = 3,  // disables the stream
    } command;
};
}  // namespace camsrv
#endif