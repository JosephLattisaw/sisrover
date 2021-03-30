#include "server.hpp"

#include <iostream>

#include "daq_message_type.hpp"

Server::Server(boost::asio::io_service& io_service, std::uint16_t p)
    : io_service(io_service),
      acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), p)),
      port(p) {}

Server::~Server() {}

void Server::start_async_accept() {
    assert(!temp_socket);  // sanity check
    temp_socket = std::make_unique<boost::asio::ip::tcp::socket>(io_service);
    acceptor.async_accept(*temp_socket, [&](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "server: accepted connection on port " << port << std::endl;
            reset();                          // resetting socket before setting it
            socket = std::move(temp_socket);  // moving socket so temporary socket can start
                                              // accepting connections again
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

void Server::reset() {
    if (socket) {
        socket->close();
        socket.release();
    }
    reset_buffers();
}

void Server::start_read() {
    assert(socket);  // sanity check
    boost::asio::async_read(
        *socket, header_buffer, boost::asio::transfer_exactly(sizeof(daqsrv::daq_message_type)),
        [&](const boost::system::error_code& error, std::size_t bytes_transferred) {
            if (!error) {
                if (header_buffer.size() == sizeof(daqsrv::daq_message_type)) {
                    const daqsrv::daq_message_type* hdr =
                        boost::asio::buffer_cast<const daqsrv::daq_message_type*>(
                            header_buffer.data());

                    switch (hdr->message_type) {
                        case daqsrv::daq_message_type::daqsrv_command::START_OFFLINE:
                            std::cout << "server: received start offline command" << std::endl;
                            data_started = true;
                            break;
                        case daqsrv::daq_message_type::daqsrv_command::START_ONLINE:
                            std::cout << "server: received start online command" << std::endl;
                            data_started = true;
                            break;
                        case daqsrv::daq_message_type::daqsrv_command::START_SCAN:
                            std::cout << "server: received start scan command" << std::endl;
                            data_started = true;
                            break;
                        case daqsrv::daq_message_type::daqsrv_command::STOP_DATA:
                            std::cout << "server: received stop data command" << std::endl;
                            data_started = false;
                            break;
                        case daqsrv::daq_message_type::daqsrv_command::DAQ_SETTINGS:
                            std::cout << "server: received daq settings" << std::endl;
                            boost::asio::async_read(
                                *socket, message_buffer,
                                boost::asio::transfer_exactly(sizeof(daqsrv::daq_settings_type)),
                                [&](const boost::system::error_code& error,
                                    std::size_t bytes_transferred) {
                                    if (!error) {
                                        if (message_buffer.size() ==
                                            sizeof(daqsrv::daq_settings_type)) {
                                            // TODO something with the settings
                                            reset_buffers();
                                            start_read();
                                        } else {
                                            std::cerr
                                                << "server: received invalid settings size of: "
                                                << bytes_transferred << ", expected: "
                                                << sizeof(daqsrv::daq_settings_type) << std::endl;
                                            reset();
                                        }
                                    } else {
                                        std::cerr
                                            << "server: encountered error when reading settings: "
                                            << error.message() << std::endl;
                                        reset();
                                    }
                                });
                            return;
                        default:
                            std::cerr << "server: received unknown command, resetting socket"
                                      << std::endl;
                            reset();
                            return;
                    }

                    reset_buffers();
                    start_read();
                } else {
                    std::cerr << "server: received invalid header size of: " << bytes_transferred
                              << ", expected: " << sizeof(daqsrv::daq_message_type) << std::endl;
                    reset();
                }
            } else {
                std::cerr << "server: encountered error when reading header: " << error.message()
                          << std::endl;
                reset();
            }
        });
}

void Server::reset_buffers() {
    header_buffer.consume(header_buffer.size());
    message_buffer.consume(message_buffer.size());
}