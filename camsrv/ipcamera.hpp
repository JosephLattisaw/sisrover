#ifndef IPCAMERA__HPP
#define IPCAMERA__HPP

// standard includes
#include <iostream>
#include <thread>

// live555 includes
#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>

#include "camera.hpp"

class Server;  // forward declaration

class IPCamera : public Camera {
public:
    IPCamera(std::shared_ptr<Server> server, boost::asio::io_service &controller_service,
             boost::asio::io_service &io_service, std::string rtsp_url);

    void get_frame(
        void *data,
        unsigned size);  // TODO, this probably shouldn't be public, only used by dummysink+

private:
    // RTSP 'response handlers'
    static void continue_after_describe(RTSPClient *client, int result, char *result_string);
    static void continue_after_setup(RTSPClient *client, int result, char *result_string);
    static void continue_after_play(RTSPClient *client, int result, char *result_string);

    // Other event handler functions

    // called when a stream's subsession (e.g., audio or video substream) ends
    static void subsession_after_playing(void *data);
    // called when a RTCP "BYE" is received for a subsession
    static void subsession_bye_handler(void *data, char const *reason);
    // called at the end of a stream's expected duration (if the stream has not already signaled its
    // end using a RTCP "BYE")
    static void stream_timer_handler(void *data);

    // The main streaming routine (for each "rtsp://" URL)
    void open_url(UsageEnvironment &environment, char const *program_name, char const *rtsp_url);
    // Used to iterate through each stream's 'subsessions', setting up each one
    static void setup_next_subsession(RTSPClient *client);
    // Used to shut down and close a stream (including its "RTSPClient" object)
    static void shutdown_stream(RTSPClient *client, int exit_code = 1);

    // live555 api
    UsageEnvironment *environment = nullptr;
    TaskScheduler *scheduler = nullptr;
};

#endif