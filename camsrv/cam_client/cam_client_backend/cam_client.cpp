#include "cam_client.hpp"

#include <iostream>

Cam_Client::Cam_Client(boost::asio::io_service &io_service, Connection_Callback conn_cb)
    : io_service(io_service),
      connection_callback{conn_cb},
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

                try {
                    boost::asio::write(
                        socket, boost::asio::buffer(reinterpret_cast<char *>(&cm), sizeof(cm)));
                } catch (const boost::exception &) {
                    std::cerr << "client: error writing keep alive, resetting socket" << std::endl;
                    reset();  // resetting socket because of error
                }

                io_service.post(std::bind(&Cam_Client::start_keepalive, this));
            }
        } else {
            std::cerr << "client: keepalive error, resetting socket" << std::endl;
            reset();  // resetting socket because of error
        }
    });
}

void Cam_Client::start_async_connect() {
    socket.async_connect(endpoint, [&](const boost::system::error_code &error) {
        if (!error) {
            std::cout << "connected successfully" << std::endl;
            updated_connection_status(true);

            start_read();

            camsrv::camsrv_message cm;
            cm.command = camsrv::camsrv_message::camsrv_command::STREAM_ON;

            try {
                boost::asio::write(socket,
                                   boost::asio::buffer(reinterpret_cast<char *>(&cm), sizeof(cm)));
            } catch (const boost::exception &) {
                std::cerr << "client: error writing stream on, resetting socket, reason: "
                          << std::endl;
                reset();
            }
        } else
            std::cerr << "client: connection failed" << std::endl;
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

    try {
        socket.close();
    } catch (const boost::exception &) {
        std::cerr << "client: error closing socket connection" << std::endl;
        io_service.post(std::bind(&Cam_Client::reset, this));
    }

    reset_buffers();
    updated_connection_status(false);

    start_async_connect();
}

void Cam_Client::reset_buffers() {
    header_buffer.consume(header_buffer.size());
    data_buffer.consume(data_buffer.size());
}

void Cam_Client::updated_connection_status(bool status) {
    // only want to update when there is a change
    if (status != connection_status) {
        connection_status = status;
        connection_callback(connection_status);
    }
}