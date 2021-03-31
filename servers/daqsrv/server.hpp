#ifndef SERVER_HPP
#define SERVER_HPP

// boost includes
#include <boost/asio.hpp>

// internal includes
#include "daq_message_type.hpp"

class Server {
public:
    using Settings_Callback = std::function<void(daqsrv::daq_settings_type)>;
    Server(boost::asio::io_service &io_service, std::uint16_t port, Settings_Callback callback);

private:
    void reset();               // resets socket connection that server was corresponding with
    void reset_buffers();       // resets buffer streams for received data
    void start_async_accept();  // starts listening for new connections on the socket
    void start_read();          // start reading data off tcp connection

    // asio objects
    boost::asio::io_service &io_service;
    boost::asio::ip::tcp::acceptor acceptor;

    // NOTE: using a temporary socket because we want to keep accepting tcp connections
    // This is only a one connection allowed server, but due to using different machines
    // we have run into the issue of deadlocking previously because we do not receive FIN
    // packet or kill signal. If a new connection kicks old connection off, so be it.
    std::unique_ptr<boost::asio::ip::tcp::socket> socket;
    std::unique_ptr<boost::asio::ip::tcp::socket> temp_socket;

    // buffers used to receive data
    boost::asio::streambuf header_buffer;
    boost::asio::streambuf message_buffer;

    bool data_started = false;  // used to keep track of if scan data should be sent to socket
    const std::uint16_t port;   // keeping track of the port number

    Settings_Callback settings_callback;  // updates the daq settings
};

#endif