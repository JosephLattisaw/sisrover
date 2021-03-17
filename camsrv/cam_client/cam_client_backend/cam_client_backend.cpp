#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>

#include "cam_client.hpp"

// Global Variables
// These are needed for the actual backend
// Creating an actual object within frontend is overtly complicated and still being developed within
// the language itself. No one should be accessing these outside of the library itself
boost::asio::io_service io_service;
std::unique_ptr<Cam_Client> cam_client;

// Wrappers are needed because of name mangling in C++
// Dart performs a dynamic lookup and can't decipher the name otherwise
extern "C" {
// C Style type definition for connection to camera server
typedef void (*Connection_Callback)(bool a);

// This creates our camera client backend and adds our callbacks
void create_cam_client(Connection_Callback connection_callback) {
    cam_client = std::make_unique<Cam_Client>(io_service, connection_callback);
}

// This destroys our camera client object
// TODO find out if this is actually needed since we are using smart pointers
void destroy_cam_client() {
    io_service.stop();
    cam_client.reset();
}

// This runs are io service
// It's important we are doing this from a different thread.
// Boost's IO service is thread safe
// TODO we need to make sure our application is completely thread safe
void run_service() {
    io_service.run();
    boost::thread t(boost::bind(&boost::asio::io_service::run, boost::ref(io_service)));
    t.detach();
}
}

void connected(bool a) { std::cout << "got a call back of conencted " << a << std::endl; }

int main(int argc, char **argv) {
    create_cam_client(connected);
    io_service.run();  // you'd call run_service instead if using c api
    destroy_cam_client();
    return 0;
}
