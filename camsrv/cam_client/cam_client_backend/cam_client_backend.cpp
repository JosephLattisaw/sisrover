#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>

#include "cam_client.hpp"

// dart api headers
#include "include/dart_api.h"
#include "include/dart_api_dl.h"
#include "include/dart_native_api.h"

// Global Variables
// These are needed for the actual backend
// Creating an actual object within frontend is overtly complicated and still being developed within
// the language itself. No one should be accessing these outside of the library itself
boost::asio::io_service io_service;
std::unique_ptr<Cam_Client> cam_client;

static void FreeFinalizer(void*, void* value) {
    std::cout << "FreeFinalizer called: " << value << std::endl;
    std::cout << std::flush;
    fflush(stdout);
    free(value);
}

// Wrappers are needed because of name mangling in C++
// Dart performs a dynamic lookup and can't decipher the name otherwise
extern "C" {
// This creates our camera client backend and adds our callbacks
void create_cam_client(int64_t connection_port, int64_t image_port) {
    cam_client = std::make_unique<Cam_Client>(
        io_service,
        [connection_port](bool status) {
            Dart_CObject dart_object;
            dart_object.type = Dart_CObject_kBool;
            dart_object.value.as_bool = status;
            Dart_PostCObject_DL(connection_port, &dart_object);
        },
        [image_port](std::vector<std::uint8_t> data) {
            void* request_buffer = malloc(sizeof(uint8_t) * data.size());
            const size_t request_length = sizeof(uint8_t) * data.size();

            Dart_CObject dart_object;
            dart_object.type = Dart_CObject_kExternalTypedData;
            dart_object.value.as_external_typed_data.type = Dart_TypedData_kUint8;
            dart_object.value.as_external_typed_data.length = request_length;
            dart_object.value.as_external_typed_data.data = static_cast<uint8_t*>(request_buffer);
            dart_object.value.as_external_typed_data.peer = request_buffer;
            dart_object.value.as_external_typed_data.callback = FreeFinalizer;
            Dart_PostCObject_DL(image_port, &dart_object);
        });
}

// This destroys our camera client object
// TODO find out if this is actually needed since we are using smart pointers
void destroy_cam_client() {
    std::cout << "destroy cam client" << std::endl;
    io_service.stop();
    cam_client.reset();
}

// This runs are io service
// It's important we are doing this from a different thread.
// Boost's IO service is thread safe
// TODO we need to make sure our application is completely thread safe
void run_service() {
    boost::thread t(boost::bind(&boost::asio::io_service::run, boost::ref(io_service)));
    t.detach();
}

DART_EXPORT intptr_t InitializeDartApi(void* data) { return Dart_InitializeApiDL(data); }
}

void connected(bool a) { std::cout << "got a call back of conencted " << a << std::endl; }

int main(int argc, char** argv) {
    /*create_cam_client(connected);
    io_service.run();  // you'd call run_service instead if using c api
    destroy_cam_client();*/
    return 0;
}
