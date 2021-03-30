#include <boost/program_options.hpp>
#include <iostream>

#include "defines.hpp"

// TODO Options should be made into a library to match similarites in camsrv
namespace {
const std::string OPTIONS_DESCRIPTION = "Options";  // options label for help menu

// creating an options table for user experience
const int OPTIONS_NUMBER_PARAMETERS = 3;
const int OPTIONS_NUMBER_ELEMENTS = 13;
const std::array<const std::array<std::string, OPTIONS_NUMBER_PARAMETERS>, OPTIONS_NUMBER_ELEMENTS>
    OPTIONS_HANDLE = {{
        {"help", "h", "Displays this help."},
        {"version", "v", "Displays version information"},
        {"port", "p", "TCP/IP port for server to listen on."},
        {"test_mode", "",
         "Test mode doesn't actually read data from FPGA and reads from file instead."},
        {"number_of_asics", "a", "Number of asics the detector arm is using."},
        {"ms_buff", "m", "Upper 7 bits of ADDCNT or the buffer address for the memory."},
        {"sample_time", "t",
         "Sample time of 4, 8, 16, 32, 64 ms which is written to PAREG register."},
        {"daq_version", "",
         "Version of QUickUSB/FPGA daq is using. (0 = old, 1 = FPGA Read/Writes)"},
        {"serial_number", "", "First serial number on QuickUSB's board."},
        {"buffer_size", "b", "Size of buffer to read data off QuickUSB FPGA."},
        {"read_multiple", "r", "Read multiples of buffer size to create less QuickUSB overhead."},
        {"test_server", "", "Test mode for server which sends off/on/raw data files."},
        {"timeout", "", "Timeout for QuickUSB requests."},
    }};

enum OPTIONS {
    HELP = 0,
    VERSION = 1,
    PORT = 2,
    TEST_MODE = 3,
    NUMBER_OF_ASICS = 4,
    MS_BUFF = 5,
    SAMPLE_TIME = 6,
    DAQ_VERSION = 7,
    SERIAL_NUMBER = 8,
    BUFFER_SIZE = 9,
    READ_MULTIPLE = 10,
    TEST_SERVER = 11,
    TIMEOUT = 12,
};

enum OPTION_HANDLES {
    HANDLE = 0,
    SHORT_HANDLE = 1,
    DESCRIPTION = 2,
};

// utility function to print version number
void print_version_number() { std::cout << daqsrv::DAQSRV_VERSION_NUMBER << std::endl; }

// utility function to get a specific options handle based on options table
std::string get_option_handle(OPTIONS options_index, OPTION_HANDLES handle_index) {
    std::string res;
    try {
        auto opt_h = OPTIONS_HANDLE.at(options_index);
        res = opt_h.at(handle_index);
    } catch (std::out_of_range const &exc) {
        std::cout << __PRETTY_FUNCTION__ << ", options_index: " << options_index
                  << ", handle_index: " << handle_index << std::endl;
        throw;
    }

    return res;
}

// utility function to get a specific options handle
std::string get_option_handles(OPTIONS index) {
    return get_option_handle(index, OPTION_HANDLES::HANDLE) + "," +
           get_option_handle(index, OPTION_HANDLES::SHORT_HANDLE);
}

// utility function to get the options description
std::string get_options_description(OPTIONS index) {
    return get_option_handle(index, OPTION_HANDLES::DESCRIPTION);
}

// utility function to get the options handle
std::string get_options_long_handle(OPTIONS index) {
    return get_option_handle(index, OPTION_HANDLES::HANDLE);
}

void required_option_check(boost::program_options::variables_map &variables_map,
                           std::string option_name) {
    auto is_opt_set = variables_map.count(option_name);
    if (!is_opt_set) {
        std::cout << "usage: " << option_name << " option needs to be set" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

template <typename T>
void required_min_max_option_check(T value, T minimum, T maximum, std::string option_name) {
    if (value < minimum || value > maximum) {
        std::cout << "usage: " << option_name << " option must be between " << minimum << " and "
                  << maximum << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

template <typename T>
void required_list_option_check(T value, std::list<T> accepted_values, std::string option_name) {
    for (const auto &av : accepted_values) {
        if (av == value) return;  // accepted value
    }

    std::cout << "usage: " << option_name << " option must be an accepted value: {";

    for (const auto &av : accepted_values) {
        std::cout << av << ",";
    }
    std::cout << "}" << std::endl;
    std::exit(EXIT_FAILURE);
}

}  // namespace

int main(int argc, char **argv) {
    namespace prog_opts = boost::program_options;

    std::uint16_t port_number;              // port number to be used for our tcp/ip server
    std::uint16_t number_of_asics = 16;     // number of asics detector arm is using
    std::uint16_t ms_buff = 119;            // upper 7 bits of ADDCNT
    std::uint16_t sample_time = 4;          // sample time of PAREG register
    std::uint16_t daq_version;              // version of daq we are using
    std::uint16_t serial_number;            // serial number of quickusb board
    std::uint32_t buffer_size = 122880;     // size of buffer to read data off quickusb board
    std::uint16_t read_multiple = 1;        // read multiples of buffer size
    std::uint32_t quickusb_timeout = 1000;  // timeout for quickusb requests

    // getting our options values. NOTE: created a separate object only for readability
    auto hlp_hdl = get_option_handles(OPTIONS::HELP);
    auto hlp_desc = get_options_description(OPTIONS::HELP);
    auto opt_hdl = get_option_handles(OPTIONS::VERSION);
    auto opt_desc = get_options_description(OPTIONS::VERSION);
    auto port_hdl = get_option_handles(OPTIONS::PORT);
    auto port_opt = prog_opts::value<decltype(port_number)>(&port_number);
    auto port_desc = get_options_description(OPTIONS::PORT);
    auto tm_hdl = get_option_handles(OPTIONS::TEST_MODE);
    auto tm_desc = get_options_description(OPTIONS::TEST_MODE);
    auto noa_hdl = get_option_handles(OPTIONS::NUMBER_OF_ASICS);
    auto noa_opt = prog_opts::value<decltype(number_of_asics)>(&number_of_asics)
                       ->default_value(number_of_asics);
    auto noa_desc = get_options_description(OPTIONS::NUMBER_OF_ASICS);
    auto mb_hdl = get_option_handles(OPTIONS::MS_BUFF);
    auto mb_opt = prog_opts::value<decltype(ms_buff)>(&ms_buff)->default_value(ms_buff);
    auto mb_desc = get_options_description(OPTIONS::MS_BUFF);
    auto st_hdl = get_option_handles(OPTIONS::SAMPLE_TIME);
    auto st_opt = prog_opts::value<decltype(sample_time)>(&sample_time)->default_value(sample_time);
    auto st_desc = get_options_description(OPTIONS::SAMPLE_TIME);
    auto dv_hdl = get_option_handles(OPTIONS::DAQ_VERSION);
    auto dv_opt = prog_opts::value<decltype(daq_version)>(&daq_version);
    auto dv_desc = get_options_description(OPTIONS::DAQ_VERSION);
    auto sn_hdl = get_option_handles(OPTIONS::SERIAL_NUMBER);
    auto sn_opt = prog_opts::value<decltype(serial_number)>(&serial_number);
    auto sn_desc = get_options_description(OPTIONS::SERIAL_NUMBER);
    auto bs_hdl = get_option_handles(OPTIONS::BUFFER_SIZE);
    auto bs_opt = prog_opts::value<decltype(buffer_size)>(&buffer_size)->default_value(buffer_size);
    auto bs_desc = get_options_description(OPTIONS::BUFFER_SIZE);
    auto rm_hdl = get_option_handles(OPTIONS::READ_MULTIPLE);
    auto rm_opt =
        prog_opts::value<decltype(read_multiple)>(&read_multiple)->default_value(read_multiple);
    auto rm_desc = get_options_description(OPTIONS::READ_MULTIPLE);
    auto ts_hdl = get_option_handles(OPTIONS::TEST_SERVER);
    auto ts_desc = get_options_description(OPTIONS::TEST_SERVER);
    auto to_hdl = get_option_handles(OPTIONS::TIMEOUT);
    auto to_opt = prog_opts::value<decltype(quickusb_timeout)>(&quickusb_timeout)
                      ->default_value(quickusb_timeout);
    auto to_desc = get_options_description(OPTIONS::TIMEOUT);

    // creating our options table
    prog_opts::options_description desc(OPTIONS_DESCRIPTION);
    desc.add_options()(hlp_hdl.c_str(), hlp_desc.c_str())(
        opt_hdl.c_str(),
        opt_desc
            .c_str())(port_hdl.c_str(), port_opt,
                      port_desc
                          .c_str())(tm_hdl.c_str(),
                                    tm_desc
                                        .c_str())(noa_hdl.c_str(), noa_opt,
                                                  noa_desc
                                                      .c_str())(mb_hdl.c_str(), mb_opt,
                                                                mb_desc
                                                                    .c_str())(st_hdl.c_str(),
                                                                              st_opt,
                                                                              st_desc
                                                                                  .c_str())(dv_hdl
                                                                                                .c_str(),
                                                                                            dv_desc
                                                                                                .c_str())(sn_hdl
                                                                                                              .c_str(),
                                                                                                          sn_desc
                                                                                                              .c_str())(bs_hdl
                                                                                                                            .c_str(),
                                                                                                                        bs_opt,
                                                                                                                        bs_desc
                                                                                                                            .c_str())(rm_hdl
                                                                                                                                          .c_str(),
                                                                                                                                      rm_opt,
                                                                                                                                      rm_desc
                                                                                                                                          .c_str())(ts_hdl
                                                                                                                                                        .c_str(),
                                                                                                                                                    ts_desc
                                                                                                                                                        .c_str())(to_hdl
                                                                                                                                                                      .c_str(),
                                                                                                                                                                  to_opt,
                                                                                                                                                                  to_desc
                                                                                                                                                                      .c_str());

    // grabbing options from command line
    prog_opts::variables_map vars_map;
    prog_opts::store(prog_opts::parse_command_line(argc, argv, desc), vars_map);
    prog_opts::notify(vars_map);

    // if user selected help option
    if (vars_map.count(get_options_long_handle(OPTIONS::HELP))) {
        print_version_number();
        std::cout << daqsrv::DAQSRV_APPLICATION_DESCRIPTION << std::endl;
        std::cout << desc << std::endl;
        std::exit(EXIT_SUCCESS);
    } else if (vars_map.count(get_options_long_handle(OPTIONS::VERSION))) {
        print_version_number();
        std::exit(EXIT_SUCCESS);
    }

    // checking required options
    required_option_check(vars_map, get_options_long_handle(OPTIONS::PORT));
    required_option_check(vars_map, get_options_long_handle(OPTIONS::DAQ_VERSION));
    required_option_check(vars_map, get_options_long_handle(OPTIONS::SERIAL_NUMBER));

    // optional set values
    bool test_mode = vars_map.count(get_options_long_handle(OPTIONS::TEST_MODE));
    bool test_server = vars_map.count(get_options_long_handle(OPTIONS::TEST_SERVER));

    // checking required minimums and maximums
    required_min_max_option_check<decltype(port_number)>(port_number, daqsrv::MINIMUM_PORT_NUMBER,
                                                         daqsrv::MAXIMUM_PORT_NUMBER,
                                                         get_options_long_handle(OPTIONS::PORT));

    // getting required items that have multiple valid answers
    required_list_option_check<decltype(sample_time)>(
        sample_time, daqsrv::VALID_SAMPLE_TIMES, get_options_long_handle(OPTIONS::SAMPLE_TIME));
    required_list_option_check<decltype(daq_version)>(
        daq_version, daqsrv::VALID_DAQ_VERSIONS, get_options_long_handle(OPTIONS::DAQ_VERSION));
}
