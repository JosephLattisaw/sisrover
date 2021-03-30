#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>

class Server {
public:
    Server(boost::asio::io_service &io_service, std::uint16_t port);
    ~Server();

private:
    void reset();               // resets socket connection that server was corresponding with
    void reset_buffers();       // resets buffer streams for received data
    void start_async_accept();  // starts listening for new connections on the socket
    void start_read();

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

    bool data_started = false;
    const std::uint16_t port;
};

#endif