#include "server.hpp"

#include <iostream>

#include "camsrv_msg.hpp"

#define KEEP_ALIVE_TIMOUT_SECONDS 15

Server::Server(boost::asio::io_service& io_service, std::uint16_t p)
    : io_service(io_service),
      acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), p)),
      port(p),
      timer(io_service) {
    start_async_accept();  // starting to accept connections
}

// opening up our temporary socket connection to start allowing for connections
void Server::start_async_accept() {
    assert(!temp_socket);  // sanity check
    temp_socket = std::make_unique<boost::asio::ip::tcp::socket>(io_service);
    acceptor.async_accept(*temp_socket, [this](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "server: accepted connection on port " << port << std::endl;
            reset();                          // resetting socket before setting it
            socket = std::move(temp_socket);  // moving socket so temporary socket can start
                                              // accepting connections again
            start_keepalive();                // starting our keepalive timer
            start_read();                     // starting to read data
        } else {
            std::cerr << "server: attempted to accept connection on port " << port
                      << " but an error occurred: " << error.message() << std::endl;
            temp_socket.release();  // just in case
        }

        // starting to accept connections again now that our connection has been processed
        start_async_accept();
    });
}

void Server::start_read() {
    assert(socket);  // sanity check
    boost::asio::async_read(
        *socket, message_buffer, boost::asio::transfer_exactly(sizeof(camsrv::camsrv_message)),
        [this](const boost::system::error_code& error, std::size_t bytes_transferred) {
            if (!error) {
                if (message_buffer.size() == sizeof(camsrv::camsrv_message)) {
                    const camsrv::camsrv_message* cm =
                        boost::asio::buffer_cast<const camsrv::camsrv_message*>(
                            message_buffer.data());

                    switch (cm->command) {
                        case camsrv::camsrv_message::camsrv_command::STREAM_ON:
                            std::cout << "server: received stream on command" << std::endl;
                            stream_on();
                            break;
                        case camsrv::camsrv_message::camsrv_command::STREAM_OFF:
                            std::cout << "server: received stream off command" << std::endl;
                            stream_off();
                            break;
                        case camsrv::camsrv_message::camsrv_command::KEEP_ALIVE:
                            std::cout << "server: received keepalive" << std::endl;
                            timer.cancel();  // cancelling keep alive expiration
                            break;
                        case camsrv::camsrv_message::camsrv_command::IMAGE:
                        default:
                            std::cout << "server: received unknown command: "
                                      << static_cast<std::uint32_t>(cm->command) << std::endl;
                            // disconnecting socket from the server because unknown command was sent
                            reset();
                            return;
                    }

                    reset_buffers();
                    start_read();
                } else {
                    std::cerr << "server: received invalid message size of: " << bytes_transferred
                              << ", expected: " << sizeof(camsrv::camsrv_message) << std::endl;
                    reset();
                }
            } else {
                std::cerr << "server: encountered error when reading header: " << error.message()
                          << std::endl;
                reset();
            }
        });
}

// resetting the socket
void Server::reset() {
    if (socket) {
        socket->close();
        socket.release();
    }
    reset_buffers();
    stream_off();
    timer.cancel();  // cancelling keep alive timer
}

// resetting the buffers
void Server::reset_buffers() { message_buffer.consume(message_buffer.size()); }

void Server::start_keepalive() {
    std::cout << "server: " << __PRETTY_FUNCTION__ << " called" << std::endl;
    timer.expires_after(std::chrono::seconds(KEEP_ALIVE_TIMOUT_SECONDS));
    timer.async_wait([this](const boost::system::error_code& error) {
        if (error == boost::asio::error::operation_aborted && socket)
            start_keepalive();
        else if (!error) {
            std::cerr << "server: did not receive keep-alive within time" << std::endl;
            reset();
        } else
            std::cerr << "server: encountered error while processing keep alive: "
                      << error.message() << std::endl;  // unknown situation, try to keep going
    });
}

void Server::send_frame(std::vector<std::uint8_t> image) {
    camsrv::camsrv_message cm;
    if (socket && socket->is_open()) {
        std::cout << "server: sending frame of " << image.size() << " bytes" << std::endl;
        cm.command = camsrv::camsrv_message::camsrv_command::IMAGE;
        cm.size = static_cast<decltype(cm.size)>(image.size());
        boost::asio::write(*socket, boost::asio::buffer(image.data(), image.size()));
    } else
        std::cerr << "server: couldn't send frame of " << image.size()
                  << " bytes, not connected to server" << std::endl;
}