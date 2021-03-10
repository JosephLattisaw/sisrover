#include "ipcamera.hpp"

#include "server.hpp"

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to
// True:
#define REQUEST_STREAMING_OVER_TCP False

#define RTSP_CLIENT_VERBOSITY_LEVEL 1  // by default, print verbose output from each "RTSPClient"
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 0
#define DEFAULT_SOCKET_NUMBER_TO_SERVER -1

// Counts how many streams (i.e., "RTSPClient"s) are currently in use.
static unsigned rtsp_client_count = 0;

static char event_loop_watch_variable = 0;
std::shared_ptr<IPCamera> ipcamera_obj;

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
class StreamClientState {
public:
    StreamClientState();
    virtual ~StreamClientState();

    MediaSubsessionIterator *iterator;
    MediaSession *session;
    MediaSubsession *subsession;
    TaskToken stream_timer_task;
    double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can
// define and use just a single "StreamClientState" structure, as a global variable in your
// application.  However, because - in this demo application - we're showing how to play multiple
// streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a
// "StreamClientState" field to the subclass:
class ourRTSPClient : public RTSPClient {
public:
    static ourRTSPClient *create_new(UsageEnvironment &environment, char const *rtsp_url,
                                     int verbosity_level = 0,
                                     char const *application_name = nullptr,
                                     portNumBits tunnel_over_http_port_num = 0);

protected:
    ourRTSPClient(UsageEnvironment &environment, char const *rtsp_url, int verbosity_level,
                  char const *application_name, portNumBits tunnel_over_http_port_num);
    // called only by createNew();

public:
    StreamClientState stream_client_state;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e.,
// each audio or video 'substream'). In practice, this might be a class (or a chain of classes) that
// decodes and then renders the incoming audio or video. Or it might be a "FileSink", for outputting
// the received data into a file (as is done by the "openRTSP" application). In this example code,
// however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.
class DummySink : public MediaSink {
public:
    static DummySink *create_new(
        UsageEnvironment &environment,
        MediaSubsession &subsession,       // identifies the kind of data that's being received
        char const *stream_id = nullptr);  // identifies the stream itself (optional)

private:
    // called only by "createNew()"
    DummySink(UsageEnvironment &environment, MediaSubsession &subsession, char const *stream_id);

    virtual ~DummySink();

    static void after_getting_frame(void *data, unsigned size, unsigned truncated_size,
                                    struct timeval presentation_time,
                                    unsigned duration_microseconds);
    void after_getting_frame(unsigned size, unsigned truncated_size,
                             struct timeval presentation_time);

private:
    virtual Boolean continuePlaying();  // redefined virtual functions

private:
    u_int8_t *receive_buffer;
    MediaSubsession &subsession;
    char *stream_id;
};

StreamClientState::StreamClientState()
    : iterator(nullptr),
      session(nullptr),
      subsession(nullptr),
      stream_timer_task(nullptr),
      duration(0.0) {}

StreamClientState::~StreamClientState() {
    delete iterator;
    if (session != nullptr) {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment &env = session->envir();  // alias

        env.taskScheduler().unscheduleDelayedTask(stream_timer_task);
        Medium::close(session);
    }
}

// Implementation of "ourRTSPClient":

ourRTSPClient *ourRTSPClient::create_new(UsageEnvironment &environment, char const *url,
                                         int verbosity_level, char const *application_name,
                                         portNumBits tunnel_over_http_port_num) {
    return new ourRTSPClient(environment, url, verbosity_level, application_name,
                             tunnel_over_http_port_num);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment &environment, char const *url, int verbosity_level,
                             char const *application_name, portNumBits tunnel_over_http_port_num)
    : RTSPClient(environment, url, verbosity_level, application_name, tunnel_over_http_port_num,
                 DEFAULT_SOCKET_NUMBER_TO_SERVER) {}

// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive
// it. Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 100000

DummySink *DummySink::create_new(UsageEnvironment &environment, MediaSubsession &ss,
                                 char const *id) {
    return new DummySink(environment, ss, id);
}

DummySink::DummySink(UsageEnvironment &environment, MediaSubsession &ss, char const *id)
    : MediaSink(environment), subsession(ss) {
    stream_id = strDup(id);
    receive_buffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

DummySink::~DummySink() {
    delete[] receive_buffer;
    delete[] stream_id;
}

Boolean DummySink::continuePlaying() {
    if (fSource == nullptr) return False;  // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called
    // later, when it arrives:
    fSource->getNextFrame(receive_buffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE, after_getting_frame, this,
                          onSourceClosure, this);
    return True;
}

void DummySink::after_getting_frame(void *data, unsigned size, unsigned truncated_size,
                                    struct timeval presentation_time,
                                    unsigned duration_microseconds) {
    DummySink *ds = reinterpret_cast<DummySink *>(data);
    ipcamera_obj->get_frame(ds->receive_buffer, size);
    ds->after_getting_frame(size, truncated_size, presentation_time);
}

void DummySink::after_getting_frame(unsigned size, unsigned truncated_size,
                                    struct timeval presentation_time) {
    // We've just received a frame of data.  (Optionally) print out information about it:
    if (DEBUG_PRINT_EACH_RECEIVED_FRAME) {
        if (stream_id != nullptr) envir() << "Stream \"" << stream_id << "\"; ";
        envir() << subsession.mediumName() << "/" << subsession.codecName() << ":\tReceived "
                << size << " bytes";
        if (truncated_size > 0) envir() << " (with " << truncated_size << " bytes truncated)";
        char uSecsStr[6 + 1];  // used to output the 'microseconds' part of the presentation time
        sprintf(uSecsStr, "%06u", static_cast<unsigned>(presentation_time.tv_usec));
        envir() << ".\tPresentation time: " << static_cast<int>(presentation_time.tv_sec) << "."
                << uSecsStr;
        if (subsession.rtpSource() != nullptr &&
            !subsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
            envir() << "!";  // mark the debugging output to indicate that this presentation time is
                             // not RTCP-synchronized
        }
#ifdef DEBUG_PRINT_NPT
        envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
        envir() << "\n";
    }

    // Then continue, to request the next frame of data:
    continuePlaying();
}

UsageEnvironment &operator<<(UsageEnvironment &environment, const RTSPClient &client);
UsageEnvironment &operator<<(UsageEnvironment &environment, const MediaSubsession &subsession);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this
// if you wish:
UsageEnvironment &operator<<(UsageEnvironment &environment, const RTSPClient &client) {
    return environment << "[URL:\"" << client.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify
// this if you wish:
UsageEnvironment &operator<<(UsageEnvironment &environment, const MediaSubsession &subsession) {
    return environment << subsession.mediumName() << "/" << subsession.codecName();
}

IPCamera::IPCamera(std::shared_ptr<Server> svr, boost::asio::io_service &controller_service,
                   boost::asio::io_service &io_service, std::string url)
    : server(svr), controller_service(controller_service), io_service(io_service), rtsp_url(url) {
    ipcamera_obj = std::shared_ptr<IPCamera>(this);
    scheduler = BasicTaskScheduler::createNew();
    environment = BasicUsageEnvironment::createNew(*scheduler);
    open_url(*environment, "camsrv", rtsp_url.c_str());  // TODO make camsrv a const

    environment->taskScheduler().doEventLoop(&event_loop_watch_variable);
}

void IPCamera::open_url(UsageEnvironment &env, char const *name, char const *url) {
    // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object
    // for each stream that we wish to receive (even if more than stream uses the same "rtsp://"
    // URL).
    RTSPClient *client = ourRTSPClient::createNew(env, url, RTSP_CLIENT_VERBOSITY_LEVEL, name);
    if (client == nullptr) {
        env << "Failed to create a RTSP client for URL \"" << url << "\": " << env.getResultMsg()
            << "\n";
        return;
    }

    ++rtsp_client_count;

    // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
    // Note that this command - like all RTSP commands - is sent asynchronously; we do not block,
    // waiting for a response. Instead, the following function call returns immediately, and we
    // handle the RTSP response later, from within the event loop:
    client->sendDescribeCommand(continue_after_describe);
}

void IPCamera::get_frame(void *data, unsigned size) {
    std::vector<std::uint8_t> sf;
    sf.resize(size);
    std::memcpy(sf.data(), data, size);

    controller_service.post(std::bind(&Server::send_frame, ipcamera_obj->server, sf));
}

void IPCamera::continue_after_describe(RTSPClient *client, int result, char *result_string) {
    do {
        UsageEnvironment &env = client->envir();  // alias
        StreamClientState &scs =
            (reinterpret_cast<ourRTSPClient *>(client))->stream_client_state;  // alias

        if (result != 0) {
            env << *client << "Failed to get a SDP description: " << result_string << "\n";
            delete[] result_string;
            break;
        }

        char *const sdp_desc = result_string;
        env << *client << "Got a SDP description:\n" << sdp_desc << "\n";

        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(env, sdp_desc);
        delete[] sdp_desc;  // because we don't need it anymore
        if (scs.session == nullptr) {
            env << *client << "Failed to create a MediaSession object from the SDP description: "
                << env.getResultMsg() << "\n";
            break;
        } else if (!scs.session->hasSubsessions()) {
            env << *client << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
            break;
        }

        // Then, create and set up our data source objects for the session.  We do this by iterating
        // over the session's 'subsessions', calling "MediaSubsession::initiate()", and then sending
        // a RTSP "SETUP" command, on each one. (Each 'subsession' will have its own data source.)
        scs.iterator = new MediaSubsessionIterator(*scs.session);
        setup_next_subsession(client);
        return;
    } while (0);

    shutdown_stream(client);  // An unrecoverable error occurred with this stream.
}

void IPCamera::setup_next_subsession(RTSPClient *client) {
    UsageEnvironment &env = client->envir();  // alias
    StreamClientState &scs =
        (reinterpret_cast<ourRTSPClient *>(client))->stream_client_state;  // alias

    scs.subsession = scs.iterator->next();
    if (scs.subsession != nullptr) {
        if (!scs.subsession->initiate()) {
            env << *client << "Failed to initiate the \"" << *scs.subsession
                << "\" subsession: " << env.getResultMsg() << "\n";
            setup_next_subsession(client);  // give up on this subsession; go to the next one
        } else {
            env << *client << "Initiated the \"" << *scs.subsession << "\" subsession (";
            if (scs.subsession->rtcpIsMuxed()) {
                env << "client port " << static_cast<int>(scs.subsession->clientPortNum());
            } else {
                env << "client ports " << static_cast<int>(scs.subsession->clientPortNum()) << "-"
                    << scs.subsession->clientPortNum() + 1;
            }
            env << ")\n";

            // Continue setting up this subsession, by sending a RTSP "SETUP" command
            client->sendSetupCommand(*scs.subsession, continue_after_setup, False,
                                     REQUEST_STREAMING_OVER_TCP);
        }
        return;
    }

    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start
    // the streaming:
    if (scs.session->absStartTime() != nullptr) {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY"
        // command:
        client->sendPlayCommand(*scs.session, continue_after_play, scs.session->absStartTime(),
                                scs.session->absEndTime());
    } else {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        client->sendPlayCommand(*scs.session, continue_after_play);
    }
}

void IPCamera::shutdown_stream(RTSPClient *client, int exit_code) {
    UsageEnvironment &env = client->envir();  // alias
    StreamClientState &scs =
        (reinterpret_cast<ourRTSPClient *>(client))->stream_client_state;  // alias

    // First, check whether any subsessions have still to be closed:
    if (scs.session != nullptr) {
        Boolean some_subsessions_were_active = False;
        MediaSubsessionIterator it(*scs.session);
        MediaSubsession *ms;

        while ((ms = it.next()) != nullptr) {
            if (ms->sink != nullptr) {
                Medium::close(ms->sink);
                ms->sink = nullptr;

                // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                if (ms->rtcpInstance() != nullptr)
                    ms->rtcpInstance()->setByeHandler(nullptr, nullptr);

                some_subsessions_were_active = True;
            }
        }

        // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
        // Don't bother handling the response to the "TEARDOWN".
        if (some_subsessions_were_active) client->sendTeardownCommand(*scs.session, nullptr);
    }

    env << *client << "Closing the stream.\n";
    Medium::close(client);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to
    // comment this out, and replace it with "eventLoopWatchVariable = 1;", so that we leave the
    // LIVE555 event loop, and continue running "main()".)
    if (--rtsp_client_count == 0) exit(exit_code);
}

void IPCamera::continue_after_setup(RTSPClient *client, int result, char *result_string) {
    do {
        UsageEnvironment &env = client->envir();  // alias
        StreamClientState &scs =
            (reinterpret_cast<ourRTSPClient *>(client))->stream_client_state;  // alias

        if (result != 0) {
            env << *client << "Failed to set up the \"" << *scs.subsession
                << "\" subsession: " << result_string << "\n";
            break;
        }

        env << *client << "Set up the \"" << *scs.subsession << "\" subsession (";
        if (scs.subsession->rtcpIsMuxed()) {
            env << "client port " << static_cast<int>(scs.subsession->clientPortNum());
        } else {
            env << "client ports " << static_cast<int>(scs.subsession->clientPortNum()) << "-"
                << scs.subsession->clientPortNum() + 1;
        }
        env << ")\n";

        // Having successfully setup the subsession, create a data sink for it, and call
        // "startPlaying()" on it. (This will prepare the data sink to receive data; the actual flow
        // of data from the client won't start happening until later, after we've sent a RTSP "PLAY"
        // command.)
        scs.subsession->sink = DummySink::create_new(env, *scs.subsession, client->url());
        if (scs.subsession->sink == nullptr) {
            env << *client << "Failed to create a data sink for the \"" << *scs.subsession
                << "\" subsession: " << env.getResultMsg() << "\n";
            break;
        }

        env << *client << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
        scs.subsession->miscPtr = client;  // a hack to let subsession handler functions get the
                                           // "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
                                           subsession_after_playing, scs.subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != nullptr) {
            scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsession_bye_handler,
                                                                    scs.subsession);
        }
    } while (0);
    delete[] result_string;

    setup_next_subsession(client);  // Set up the next subsession, if any
}

void IPCamera::continue_after_play(RTSPClient *client, int result, char *result_string) {
    Boolean res = False;

    do {
        UsageEnvironment &env = client->envir();  // alias
        StreamClientState &scs =
            (reinterpret_cast<ourRTSPClient *>(client))->stream_client_state;  // alias

        if (client != 0) {
            env << *client << "Failed to start playing session: " << result_string << "\n";
            break;
        }

        // Set a timer to be handled at the end of the stream's expected duration (if the stream
        // does not already signal its end using a RTCP "BYE").  This is optional.  If, instead, you
        // want to keep the stream active - e.g., so you can later 'seek' back within it and do
        // another RTSP "PLAY" - then you can omit this code. (Alternatively, if you don't want to
        // receive the entire stream, you could set this timer for some shorter value.)
        if (scs.duration > 0) {
            // number of seconds extra to delay, after the stream's
            // expected duration.  (This is optional.)
            unsigned const _secs_after_delay = 2;
            scs.duration += _secs_after_delay;

            unsigned _usecs_to_delay = static_cast<unsigned>((scs.duration * 1000000));
            scs.stream_timer_task = env.taskScheduler().scheduleDelayedTask(
                _usecs_to_delay, reinterpret_cast<TaskFunc *>(stream_timer_handler), client);
        }

        env << *client << "Started playing session";
        if (scs.duration > 0) env << " (for up to " << scs.duration << " seconds)";
        env << "...\n";

        res = True;
    } while (0);
    delete[] result_string;

    // An unrecoverable error occurred with this stream.
    if (!res) shutdown_stream(client);
}

void IPCamera::subsession_after_playing(void *data) {
    MediaSubsession *ms = reinterpret_cast<MediaSubsession *>(data);
    RTSPClient *cl = reinterpret_cast<RTSPClient *>(ms->miscPtr);

    // Begin by closing this subsession's stream:
    Medium::close(ms->sink);
    ms->sink = nullptr;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession &sesh = ms->parentSession();
    MediaSubsessionIterator it(sesh);
    while ((ms = it.next()) != nullptr) {
        if (ms->sink != nullptr) return;  // this subsession is still active
    }

    // All subsessions' streams have now been closed, so shutdown the client:
    shutdown_stream(cl);
}

void IPCamera::subsession_bye_handler(void *data, char const *reason) {
    MediaSubsession *ms = reinterpret_cast<MediaSubsession *>(data);
    RTSPClient *cl = reinterpret_cast<RTSPClient *>(ms->miscPtr);
    UsageEnvironment &env = cl->envir();  // alias

    env << *cl << "Received RTCP \"BYE\"";
    if (reason != nullptr) {
        env << " (reason:\"" << reason << "\")";
        delete[] reason;
    }
    env << " on \"" << *ms << "\" subsession\n";

    subsession_after_playing(ms);  // Now act as if the subsession had closed
}

void IPCamera::stream_timer_handler(void *data) {
    ourRTSPClient *cl = reinterpret_cast<ourRTSPClient *>(data);
    StreamClientState &scs = cl->stream_client_state;  // alias

    scs.stream_timer_task = nullptr;

    shutdown_stream(cl);  // Shut down the stream
}