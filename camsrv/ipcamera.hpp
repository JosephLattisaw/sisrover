#ifndef IPCAMERA__HPP
#define IPCAMERA__HPP

// standard includes
#include <iostream>
#include <thread>

// boost includes
#include <boost/asio.hpp>

// live555 includes
#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>

#define RTSP_CLIENT_VERBOSITY_LEVEL 1  // by default, print verbose output from each "RTSPClient"
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 0

// forward declarations
class DummySink;
class Server;

class IPCamera {
public:
    IPCamera(std::shared_ptr<Server> server, boost::asio::io_service &controller_service,
             boost::asio::io_service &io_service, std::string rtsp_url);
    ~IPCamera();

    void get_frame(void *data, unsigned size);  // used by dummysink

private:
    // RTSP 'response handlers'
    static void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString);
    static void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString);
    static void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString);

    // Other event handler functions
    static void subsessionAfterPlaying(void *clientData);  // called when a stream's subsession
                                                           // (e.g., audio or video substream) ends
    static void subsessionByeHandler(
        void *clientData,
        char const *reason);  // called when a RTCP "BYE" is received for a subsession
    static void streamTimerHandler(
        void *clientData);  // called at the end of a stream's expected duration (if the stream has
                            // not already signaled its end using a RTCP "BYE")

    void openURL(UsageEnvironment &env, char const *progName,
                 char const *rtspURL);  // The main streaming routine (for each "rtsp://" URL)
    static void setupNextSubsession(
        RTSPClient *rtspClient);  // Used to iterate through each stream's 'subsessions', setting up
                                  // each one
    static void shutdownStream(RTSPClient *rtspClient,
                               int exitCode = 1);  // Used to shut down and close a stream
                                                   // (including its "RTSPClient" object)

    boost::asio::io_service &controller_service;  // controller object threads service
    boost::asio::io_service &io_service;          // service used by IPCamera object

    UsageEnvironment *env = nullptr;
    TaskScheduler *scheduler = nullptr;

    std::shared_ptr<Server> server;  // reference to server to be able to send data

    std::string rtsp_url;
};

#endif