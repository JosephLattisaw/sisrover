#ifndef server__HPP
#define server__HPP

#include <boost/asio.hpp>

#ifndef BOOST_ALLOW_DEPRECATED_HEADERS
#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <boost/signals2.hpp>
#undef BOOST_ALLOW_DEPRECATED_HEADERS
#else
#include <boost/signals2.hpp>
#endif

class Server {
public:
    Server(boost::asio::io_service &io_service, std::uint16_t port);

    void send_frame(std::vector<std::uint8_t> image);

protected:
    boost::signals2::signal<void()> stream_on;
    boost::signals2::signal<void()> stream_off;

private:
    void reset();
    void reset_buffers();
    void start_async_accept();
    void start_keepalive();
    void keepalive_async_wait(const boost::system::error_code &error);
    void start_read();
    void start_read_header(
        const boost::system::error_code &error,
        std::size_t bytes_transferred);  // TODO the server doesn't actuall have a header (rename)
    void handle_accept(const boost::system::error_code
                           &error);  // TODO probably could make this lambda vs global function

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
    const std::uint16_t _port;  // TODO fix naming convention
};

#endif