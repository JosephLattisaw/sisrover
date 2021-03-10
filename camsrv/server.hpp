#ifndef server__HPP
#define server__HPP

// boost includes
#include <boost/asio.hpp>

// needed to avoid pragma message warning
// warning is completely contained within boost's includes
#ifndef BOOST_ALLOW_DEPRECATED_HEADERS
#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <boost/signals2.hpp>
#undef BOOST_ALLOW_DEPRECATED_HEADERS
#else
// just in case someone wants to define the macro
#include <boost/signals2.hpp>
#endif

#include <iostream>

class Server {
public:
    Server(boost::asio::io_service &io_service, std::uint16_t port);

    void send_frame(std::vector<std::uint8_t> image);

protected:
    boost::signals2::signal<void()> stream_on;
    boost::signals2::signal<void()> stream_off;

private:
    void reset();               // resets socket connection that server was corresponding with
    void reset_buffers();       // resets buffer stream for received data
    void start_async_accept();  // starts listening for new connections on socket
    void start_keepalive();     // starts keep-alive timer
    void start_read();

    // NOTE: using a temporary socket because we want to keep accepting tcp connections
    // This is only a one connection allowed server, but due to using different machines
    // we have run into the issue of deadlocking previously because we do not receive FIN
    // packet or kill signal. If a new connection kicks old connection off, so be it.
    std::unique_ptr<boost::asio::ip::tcp::socket> socket;
    std::unique_ptr<boost::asio::ip::tcp::socket> temp_socket;

    // asio objects
    boost::asio::io_service &io_service;
    boost::asio::ip::tcp::acceptor acceptor;

    // keep-alive
    boost::asio::steady_timer timer;  // keep-alive timer

    boost::asio::streambuf message_buffer;
    const std::uint16_t port;
};

#endif