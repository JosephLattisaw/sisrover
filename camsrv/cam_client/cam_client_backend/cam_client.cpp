#include "cam_client.hpp"

#include <iostream>

Cam_Client::Cam_Client(boost::asio::io_service &io_service)
    : io_service(io_service),
      socket(io_service),
      endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 20000),
      timer(io_service) {
    start_keepalive();
    start_async_connect();
}

void Cam_Client::start_keepalive() {
    timer.expires_after(std::chrono::seconds(2));
    timer.async_wait([&](const boost::system::error_code &error) {
        if (!error) {
            if (socket.is_open()) {
                std::cout << "client: sending keepalive" << std::endl;
                camsrv::camsrv_message cm;
                cm.command = camsrv::camsrv_message::camsrv_command::KEEP_ALIVE;
                boost::asio::write(socket,
                                   boost::asio::buffer(reinterpret_cast<char *>(&cm), sizeof(cm)));
            }
        } else {
            std::cerr << "client: keepalive error, resetting socket" << std::endl;
            reset();
        }

        start_keepalive();
    });
}

void Cam_Client::start_async_connect() {
    socket.async_connect(endpoint, [&](const boost::system::error_code &error) {
        if (!error) {
            std::cout << "connected successfully" << std::endl;
            start_read();

            camsrv::camsrv_message cm;
            cm.command = camsrv::camsrv_message::camsrv_command::STREAM_ON;
            boost::asio::write(socket,
                               boost::asio::buffer(reinterpret_cast<char *>(&cm), sizeof(cm)));
        } else
            std::cout << "connection failed" << std::endl;
    });
}

void Cam_Client::start_read() {
    boost::asio::async_read(
        socket, header_buffer, boost::asio::transfer_exactly(sizeof(camsrv::camsrv_message)),
        [&](const boost::system::error_code &error, std::size_t bytes_transferred) {
            if (!error) {
                if (header_buffer.size() == sizeof(camsrv::camsrv_message)) {
                    const camsrv::camsrv_message *cm =
                        boost::asio::buffer_cast<const camsrv::camsrv_message *>(
                            header_buffer.data());

                    switch (cm->command) {
                        case camsrv::camsrv_message::camsrv_command::IMAGE:
                            std::cout << "client: received image" << std::endl;

                            boost::asio::async_read(
                                socket, data_buffer, boost::asio::transfer_exactly(cm->size),
                                [&, cm](const boost::system::error_code &error,
                                        std::size_t bytes_transferred) {
                                    if (!error) {
                                        if (bytes_transferred == cm->size) {
                                            std::cout << "client: received image size of "
                                                      << bytes_transferred << std::endl;
                                            reset_buffers();
                                            start_read();
                                        } else {
                                            std::cerr << "client: received invalid image size of: "
                                                      << bytes_transferred
                                                      << ", expected: " << sizeof(cm->size)
                                                      << std::endl;
                                            reset();
                                        }
                                    } else {
                                        std::cerr
                                            << "client: encountered error when reading image: "
                                            << error.message() << std::endl;
                                        reset();
                                    }
                                });
                            break;
                        default:
                            std::cerr << "client: received unknown command: "
                                      << static_cast<std::uint32_t>(cm->command) << std::endl;
                            reset();
                    }
                } else {
                    std::cerr << "client: received invalid header size of: " << bytes_transferred
                              << ", expected: " << sizeof(camsrv::camsrv_message) << std::endl;
                    reset();
                }
            } else {
                std::cerr << "client: encountered error when reading header: " << error.message()
                          << std::endl;
                reset();
            }
        });
}

void Cam_Client::reset() {
    std::cout << "client: resetting socket" << std::endl;
    socket.close();
    reset_buffers();
    start_async_connect();
}

void Cam_Client::reset_buffers() {
    header_buffer.consume(header_buffer.size());
    data_buffer.consume(data_buffer.size());
}