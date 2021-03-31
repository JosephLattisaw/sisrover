#include "controller.hpp"

// C includes
#include <math.h>

// standard includes
#include <fstream>
#include <iostream>
#include <iterator>

// internal includes
#include "defines.hpp"

Controller::Controller(daqsrv::controller_options_type &controller_options,
                       boost::asio::io_service &io_service, Data_Callback callback)
    : test_mode(controller_options.test_mode),
      quickusb_timeout(controller_options.quickusb_timeout),
      daq_version(controller_options.daq_version),
      number_of_asics(controller_options.number_of_asics),
      buffer_size(controller_options.buffer_size),
      ms_buff(controller_options.ms_buff),
      sample_time(controller_options.sample_time),
      read_multiple(controller_options.read_multiple),
      io_service(io_service),
      timer(io_service),
      data_callback{callback} {
    serial_number = std::to_string(controller_options.serial_number).c_str()[0];

    {
        assert(test_data.str().length() == 0);  // sanity check
        std::ifstream file("data.dat");
        test_data << file.rdbuf();
        verify_truth(test_data.str().length() > 0, __PRETTY_FUNCTION__, "failed to open data.dat");
    }

    quickusb = std::make_shared<squsb::squsb>();
    std::cout << "controller: using sis_quick_usb version number: "
              << quickusb->get_version_number() << std::endl;
}

Controller::~Controller() {
    std::cout << "controller: disconnecting from QuickUsb" << std::endl;
    quickusb->disconnect_from_qusb();
}

void Controller::connect_to_daq() {
    quickusb->connect_to_qusb(serial_number, quickusb_timeout);
    setup_daq();
}

void Controller::setup_daq() {
    set_default_quickusb_settings();

    switch (daq_version) {
        case DAQ_VERSIONS::VERSION_1:
            std::cout << "controller: using daq version 1" << std::endl;
            initialize_daq_version_1();
            break;
        case DAQ_VERSIONS::VERSION_2:
            std::cout << "controller: using daq version 2" << std::endl;
            initialize_daq_version_2();
            break;
        default:
            std::cerr << "controller: unaccounted for daq version" << std::endl;
            std::exit(EXIT_FAILURE);
    }

    connected = true;

    std::cout << "controller: starting to scan" << std::endl;
    start_scanning();
}

void Controller::set_default_quickusb_settings() {
    std::cout << "controller: setting default quickusb settings" << std::endl;

    // setting settings for first address
    const auto ADR_1 = 1;
    auto t_1 = quickusb->read_quickusb_setting(ADR_1);
    verify_truth(std::get<0>(t_1), __PRETTY_FUNCTION__, "failed to read quickusb setting 1");
    auto stg_1 = std::get<1>(t_1);
    stg_1 &= ~1;
    verify_truth(quickusb->write_quickusb_setting(ADR_1, stg_1), __PRETTY_FUNCTION__,
                 "failed to write quickusb setting 1");

    // sanity check that it worked
    auto t_2 = quickusb->read_quickusb_setting(ADR_1);
    verify_truth(std::get<0>(t_2), __PRETTY_FUNCTION__,
                 "failed to write quickusb setting 1 sanity");
    verify_truth(stg_1 = std::get<1>(t_2), __PRETTY_FUNCTION__, "settings 1 don't match?");

    // setting settings for second address
    const auto ADR_2 = 2;
    auto t_3 = quickusb->read_quickusb_setting(ADR_2);
    verify_truth(std::get<0>(t_3), __PRETTY_FUNCTION__, "failed to read quickusb setting 2");
    auto stg_2 = std::get<1>(t_3);
    stg_2 &= 0xFE00;
    stg_2 |= 0xC000;
    verify_truth(quickusb->write_quickusb_setting(ADR_2, stg_2), __PRETTY_FUNCTION__,
                 "failed to write quickusb setting 2");

    // sanity check that it worked
    auto t_4 = quickusb->read_quickusb_setting(ADR_2);
    verify_truth(std::get<0>(t_4), __PRETTY_FUNCTION__, "failed to read quickusb setting 2 sanity");
    verify_truth(stg_2 == std::get<1>(t_4), __PRETTY_FUNCTION__, "settings 2 don't match?");
}

// this function is used for initializing the pre March 2017 daqsrv board
void Controller::initialize_daq_version_1() {
    // A String of 13 bytes is written into the DAQ board to set it up and start the data
    // acquisition from the detectors or the sensors Read commands over the USB are issued to read
    // large chunks (1MB to 64MB range typically) and the data is passed onto the Image API directly
    // or over TCP/IP

    daqv1_start_commanding();
    daqv1_set_number_of_asics(number_of_asics);
    daqv1_set_array_mask(1);
    daqv1_set_buffer_size(buffer_size);
    daqv1_set_array_a_timing(sample_time);
    daqv1_reset();

    quickusb->write_quickusb_command(0xC000, &mo_command_data[0], mi_data_len);
}

// this function is used for initializing the daqsrv board created around march 2017
// The main difference is that the new board uses fpga read/writes to communicate
void Controller::initialize_daq_version_2() {
    // setting number of asics
    // ASICNUM (address 8 or 0x08) = 8 bits (could be 6 bits): value range = 0 to 31
    // ASICNUM = Number of ASICS - 1
    // For example, for 16 ASICS, the value to be written into ASICNUM = 16-1 = 15, and 13 for 14
    // ASICS. NOTE: Number of ASICS should always be even and therefore ASICNUM will always be odd.

    // asics written to register is always desired number minus 1 according to Satpals doc
    auto nbr_asics = number_of_asics - 1;
    auto t = quickusb->read_fpga(ASICS_NUMBER_ADDRESS);
    verify_truth(std::get<0>(t), __PRETTY_FUNCTION__, "failed reading asics value");
    verify_truth(quickusb->write_fpga(ASICS_NUMBER_ADDRESS, nbr_asics), __PRETTY_FUNCTION__,
                 "failed writing asics value");

    // reading number of asics to make sure it got set correctly
    t = quickusb->read_fpga(ASICS_NUMBER_ADDRESS);
    verify_truth(std::get<0>(t), __PRETTY_FUNCTION__, "failed read fpga: asics");
    verify_truth(std::get<1>(t), __PRETTY_FUNCTION__, "failed to set the number of asics value");
    std::cout << "squsb: successful setting the number of asics" << std::get<1>(t) << std::endl;

    // setting msbuff value
    // MSBUFF (address = 0 or 0x09) = upper 7 bits of ADDCNT or Buffer Address for the memory, Lower
    // 10 bits will always be 0 Buffer size will be in multiples of 1024. A value of 0 means buffer
    // size of 1024 and not 0! So the buffer size = 1024 * (MSBUFF + 1) It must also be a multiple
    // of frame size
    verify_truth(quickusb->write_fpga(MSBUFF_ADDRESS, ms_buff), __PRETTY_FUNCTION__,
                 "failed to write msbuff");

    // reading msbuff to make sure it got set correctly
    t = quickusb->read_fpga(MSBUFF_ADDRESS);
    verify_truth(std::get<0>(t), __PRETTY_FUNCTION__, "failed to read fpga: msbuff");
    verify_truth(std::get<1>(t) == ms_buff, __PRETTY_FUNCTION__,
                 "failed to set msbuff " + std::to_string(ms_buff));

    // setting pareg value
    // PAREG (address = 10 or 0x0a)
    // PAREG[2:0] = 3 bits set the Ts value to 4ms, 8ms, 16ms, 32ms, 64ms
    // This value is the rate at which the fpga samples and writes data. Data will be scanned at a
    // slower rate with a higher ms selected 0b000 = 4ms 0b001 = 8ms 0b010 = 16ms 0b011 = 32ms 0b100
    // = 64ms
    auto pareg = get_pareg(sample_time);
    verify_truth(quickusb->write_fpga(PAREG_ADDRESS, pareg), __PRETTY_FUNCTION__,
                 "failed to write to fpga: pareg");

    // reading pareg to make sure it got set correctly
    t = quickusb->read_fpga(PAREG_ADDRESS);
    verify_truth(std::get<0>(t), __PRETTY_FUNCTION__, "failed to read fpga: pareg");
    verify_truth(std::get<1>(t) == pareg, __PRETTY_FUNCTION__,
                 "failed to set msbuff " + std::to_string(pareg));
    std::cout << "controller: successful set pareg " << static_cast<int>(pareg) << std::endl;

    // setting config value
    // CONFIG[7:0] (address = 11 or 0x0b)
    //[0] = 0 = Parallel bus(8 bit data, 8 bit address) bus interface to Cypress using Block
    // Handshake IO Model [0] = 1 = SPI mode interface to external world [7:6] = 0's for now =
    // spares
    auto config_val = DEFAULT_CONFIGURATION_VALUE;
    verify_truth(quickusb->write_fpga(CONFIGURATION_ADDRESS, config_val), __PRETTY_FUNCTION__,
                 "failed to write fpga: config value");

    // reading config value to make sure it got set correctly
    t = quickusb->read_fpga(CONFIGURATION_ADDRESS);
    verify_truth(std::get<0>(t), __PRETTY_FUNCTION__, "failed to read fpga: config");
    verify_truth(std::get<1>(t) == config_val, __PRETTY_FUNCTION__,
                 "failed to set config " + std::to_string(config_val));
    std::cout << "controller: successful set config value " << static_cast<int>(config_val)
              << std::endl;

    // setting start reset address
    // STRT_RST (address 15 or 0x0f)
    // = 1 Resets the FPGA
    // = 2 Starts the data acquisition by the FPGA from the sensors
    verify_truth(quickusb->write_fpga(START_RESET_ADDRESS, START_RESET_START_DATA_ACQ_VALUE),
                 __PRETTY_FUNCTION__, "failed to write fpga: start reset address");
    t = quickusb->read_fpga(START_RESET_ADDRESS);

    // reading the start reset value to make sure it got set correctly
    verify_truth(std::get<0>(t), __PRETTY_FUNCTION__, "failed to read fpga: start reset address");
    verify_truth(std::get<1>(t) == START_RESET_START_DATA_ACQ_VALUE, __PRETTY_FUNCTION__,
                 "failed to set start reset " + std::to_string(START_RESET_START_DATA_ACQ_VALUE));
}

void Controller::start_scanning() {
    if (connected) {
        std::uint8_t buff[buffer_size * read_multiple];
        QULONG l = sizeof(buff);
        auto res = quickusb->read_data(&buff[0], &l);

        if (!test_mode) {
            if (res) {
                data_callback(std::move(
                    std::vector<std::uint8_t>(buff, buff + (sizeof(buff) / sizeof(buff[0])))));
                start_scan_timer(0);  // immediately post (the read will take a second or so)
                return;
            } else
                std::cout
                    << "controller: failed to successfully read quickusb data, adding delay of 1s"
                    << std::endl;
        } else {
            data_callback(
                std::vector<std::uint8_t>((std::istream_iterator<std::uint8_t>(test_data)),
                                          std::istream_iterator<std::uint8_t>()));
        }

        // if we make it this far this fast, we want to hold off a bit for realism
        start_scan_timer(1);
    } else
        std::cerr << "controller: start scanning requested but daq is not connected?" << std::endl;
}

void Controller::start_scan_timer(unsigned int seconds) {
    timer.expires_after(std::chrono::seconds(seconds));
    timer.async_wait([&](const boost::system::error_code &error) {
        if (!error) start_scanning();
    });
}

void Controller::verify_truth(bool truth, std::string function_name, std::string error) {
    if (!truth) {
        std::cerr << "controller: failure " << function_name << ", error: " << error << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void Controller::daqv1_start_commanding() { mi_data_len = 0; }

void Controller::daqv1_send_command(COMMAND command, unsigned char arg) {
    mo_command_data[mi_data_len++] = ((command << 3) | (arg & 0x07)) & 0xff;
}

void Controller::daqv1_set_array_a_timing(int milliseconds) {
    int log = ::log2l(milliseconds);
    daqv1_send_command(COMMAND::TA, three_bits(log - 1, 0));
}

void Controller::daqv1_set_array_mask(unsigned int mask) {
    auto mi_num_arr = 0;

    for (auto x = mask; x; x >> 1) {
        if (x & 0x01) mi_num_arr++;
    }

    mask = ~mask;

    daqv1_send_command(ARRAY_MASK20, three_bits(mask, 0));
    daqv1_send_command(ARRAY_MASK53, three_bits(mask, 3));
    daqv1_send_command(ARRAY_MASK86, three_bits(mask, 6));
    daqv1_send_command(ARRAY_MASK119, three_bits(mask, 9));
}

void Controller::daqv1_set_buffer_size(int size) {
    size--;

    for (auto i = 5; i >= 0; i--) {
        auto bits = three_bits(size, i * 3);
        daqv1_send_command(COMMAND::ADDCNT, bits);
    }
}

void Controller::daqv1_set_number_of_asics(int nbr_asics) {
    nbr_asics--;

    auto high_order_3bits = three_bits(nbr_asics, 3);
    auto low_order_3bits = three_bits(nbr_asics, 0);

    daqv1_send_command(COMMAND::ASICNUM, high_order_3bits);
    daqv1_send_command(COMMAND::ASICNUM, low_order_3bits);
}

void Controller::daqv1_reset() { daqv1_send_command(COMMAND::SOFT_RESET, 0); }

char Controller::get_pareg(unsigned int sample_time) {
    if (daqsrv::SAMPLE_TIME_MAP.find(sample_time) == daqsrv::SAMPLE_TIME_MAP.end())
        verify_truth(false, __PRETTY_FUNCTION__, "invalid sample time used for pareg");

    return daqsrv::SAMPLE_TIME_MAP.at(sample_time);
}

unsigned char Controller::three_bits(unsigned long from, int first_bit) {
    return (from >> first_bit) & 0x07;
};

void Controller::update_settings(daqsrv::daq_settings_type daq_settings) {
    timer.cancel();  // cancelling timer

    buffer_size = daq_settings.buffer_size;
    ms_buff = daq_settings.ms_buff;
    number_of_asics = daq_settings.number_of_asics;
    quickusb_timeout = daq_settings.quickusb_timeout;
    read_multiple = daq_settings.read_multiple;
    sample_time = daq_settings.timing;

    std::cout << "controller: received settings update" << std::endl;
    std::cout << "controller: buffer_size: " << buffer_size
              << ", ms_buff: " << static_cast<int>(ms_buff)
              << ", number_of_asics: " << static_cast<int>(number_of_asics)
              << ", qusb_timeout: " << quickusb_timeout << ", read_multiple: " << read_multiple
              << ", sample_time: " << sample_time << std::endl;

    setup_daq();
    start_scan_timer(0);  // start scan timer immediately after settings update
}