#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>

class Server {
public:
    using Client_Disconnect_Callback = std::function<void()>;
    using Command_Callback = std::function<void(std::int32_t command, std::int32_t value)>;
    Server(boost::asio::io_service &io_service, std::uint16_t port,
           Client_Disconnect_Callback client_disconnect_callback,
           Command_Callback command_callback);
    ~Server();

private:
    void reset();
    void reset_buffers();
    void start_async_accept();
    void start_read();

    // NOTE: using a temporary socket because we want to keep accepting tcp connections
    // This is only a one connection allowed server, but due to using different machines
    // we have run into the issue of deadlocking previously because we do not receive FIN
    // packet or kill signal. If a new connection kicks old connection off, so be it.
    std::unique_ptr<boost::asio::ip::tcp::socket> socket;
    std::unique_ptr<boost::asio::ip::tcp::socket> temp_socket;

    boost::asio::io_service &io_service;
    boost::asio::ip::tcp::acceptor acceptor;

    const std::uint16_t port;

    boost::asio::streambuf header_buffer;
    boost::asio::streambuf message_buffer;

    Client_Disconnect_Callback client_disconnect_callback;
    Command_Callback command_callback;
};

#endif