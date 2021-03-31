#ifndef DAQ_HPP
#define DAQ_HPP

// boost includes
#include <boost/asio.hpp>

// standard includes
#include <cstdint>
#include <thread>

// internal includes
#include "controller.hpp"
#include "daq_message_type.hpp"
#include "defines.hpp"
#include "server.hpp"

class DAQ {
public:
    DAQ(std::uint16_t port, daqsrv::controller_options_type &controller_options,
        boost::asio::io_service &io_service);
    ~DAQ();

private:
    void worker_thread(daqsrv::controller_options_type &controller_options);

    // services
    boost::asio::io_service controller_service;  // used by our worker thread controller object
    boost::asio::io_service &io_service;         // our main io service

    // objects
    std::shared_ptr<Controller> controller;  // quickusb controller
    std::shared_ptr<Server> server;          // tcp server

    // daq settings
    daqsrv::daq_settings_type daq_settings;  // keeping track of settings
    // just keeping track if we ever update so if we ever have a situation where the server object
    // were to be created and connected and sent a settings update before we create our controller
    // object in a different thread, we'd know to update
    bool daq_settings_updated = false;

    std::thread controller_thread;  // thread for our quickusb controller
};

#endif