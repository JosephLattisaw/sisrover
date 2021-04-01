#include "server.hpp"

#include <iostream>

Server::Server(boost::asio::io_service& io_service, std::uint16_t p,
               Client_Disconnect_Callback cd_callback, Command_Callback cmd_callback)
    : io_service(io_service),
      acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), p)),
      port(p),
      client_disconnect_callback{cd_callback},
      command_callback{cmd_callback} {
    start_async_accept();
}

Server::~Server() {}

void Server::start_async_accept() {
    assert(!temp_socket);  // sanity check
    temp_socket = std::make_unique<boost::asio::ip::tcp::socket>(io_service);
    acceptor.async_accept(*temp_socket, [&](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "server: accepted connection on port " << port << std::endl;
            reset();
            socket = std::move(temp_socket);
            start_read();
        } else {
            std::cerr << "server: attempted to accept connection on port " << port
                      << " but an error occurred: " << error.message() << std::endl;
            temp_socket.release();  // just in case
        }

        // starting to accept connections again now that our connection has been processed
        start_async_accept();
    });
}

void Server::reset() {
    if (socket) {
        socket->close();
        socket.release();
    }
    reset_buffers();

    client_disconnect_callback();
}

void Server::reset_buffers() {
    header_buffer.consume(header_buffer.size());
    message_buffer.consume(message_buffer.size());
}

// TODO put actual buffer size instead of 5
void Server::start_read() {
    assert(socket);
    boost::asio::async_read(
        *socket, header_buffer, boost::asio::transfer_exactly(5),
        [&](const boost::system::error_code& error, std::size_t bytes_transferred) {
            if (!error) {
                if (bytes_transferred == 5) {
                    // TODO read actual header

                    reset_buffers();
                    start_read();
                } else {
                    std::cerr << "server: received invalid message sizer of: " << bytes_transferred
                              << ", expected: " << 5 << std::endl;
                    reset();
                }
            } else {
                std::cerr << "server: encountered error when reading header: " << error.message()
                          << std::endl;
                reset();
            }
        });
}