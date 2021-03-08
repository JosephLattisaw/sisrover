#include <boost/program_options.hpp>
#include <iostream>

#include "controller.hpp"

#define MIN_PORT 2000
#define MAX_PORT 65536

namespace {
const std::string CAMSRV_VERSION_NUMBER = "camsrv 4.0.0";  // version number
const std::string CAMSRV_APPLICATION_DESCRIPTION =
    "camera server which feeds webcam images from remote cpu";  // application description

const std::string OPTIONS_DESCRIPTION = "Options";  // options label for help menu
const int OPTION_HANDLES_COMBO = 2;

// Creating an options table for user experience
const int OPTIONS_NUMBER_PARAMS = 3;
const int OPTIONS_NUMBER_ELEMENTS = 5;
const std::array<const std::array<std::string, OPTIONS_NUMBER_PARAMS>, OPTIONS_NUMBER_ELEMENTS>
    OPTIONS_HANDLE = {{
        {"help", "h", "Displays this help."},
        {"version", "v", "Displays version information"},
        {"device_name", "d", "Device name/path for a camera (e.g. /dev/video0)"},
        {"port", "p", "TCP/IP port for server to listen on."},
        {"url", "u", "URL for RTSP to IP Camera."},
    }};

// enumeration of options
enum OPTIONS {
    HELP = 0,
    VERSION = 1,
    DEVICE_NAME = 2,
    PORT = 3,
    URL = 4,
};

// enumeration of option parameters
enum OPTION_HANDLES {
    HANDLE = 0,
    SHORT_HANDLE = 1,
    DESCRIPTION = 2,
};

// utility function to print version number
void print_version_number() { std::cout << CAMSRV_VERSION_NUMBER << std::endl; }

// TODO consolidate both of these functions into one function
// utility function to get a specific options handle
std::string get_option_handles(OPTIONS index) {
    std::string res;

    try {
        auto opt_h = OPTIONS_HANDLE.at(index);
        res = opt_h.at(OPTION_HANDLES::HANDLE) + "," + opt_h.at(OPTION_HANDLES::SHORT_HANDLE);
    } catch (std::out_of_range const &exc) {
        std::cout << __PRETTY_FUNCTION__ << ", index: " << index << std::endl;
        throw;
    }

    return res;
}

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

// TODO consolidate both of the below functions into one
// utility function to get the options description
std::string get_options_description(OPTIONS index) {
    return get_option_handle(index, OPTION_HANDLES::DESCRIPTION);
}

// utility function to get the options handle
std::string get_options_long_handle(OPTIONS index) {
    return get_option_handle(index, OPTION_HANDLES::HANDLE);
}
}  // namespace

int main(int argc, char **argv) {
    namespace prog_opts = boost::program_options;  // consolidating our namespace naming convention
    std::string device_name;                       // device name for our webcamera
    std::uint16_t port_number;                     // port number to be used for our server
    std::string url;                               // url to be used for our ip camera

    // Getting our options values. NOTE: created a separate object only for readability
    auto hlp_hdl = get_option_handles(OPTIONS::HELP).c_str();
    auto hlp_desc = get_options_description(OPTIONS::HELP).c_str();
    auto opt_hdl = get_option_handles(OPTIONS::VERSION).c_str();
    auto opt_desc = get_options_description(OPTIONS::VERSION).c_str();
    auto dev_hdl = get_option_handles(OPTIONS::DEVICE_NAME).c_str();
    auto dev_opt = prog_opts::value<decltype(device_name)>(&device_name);
    auto dev_desc = get_options_description(OPTIONS::DEVICE_NAME).c_str();
    auto port_hdl = get_option_handles(OPTIONS::PORT).c_str();
    auto port_opt = prog_opts::value<decltype(port_number)>(&port_number);
    auto port_desc = get_options_description(OPTIONS::PORT).c_str();
    auto url_hdl = get_option_handles(OPTIONS::URL).c_str();
    auto url_opt = prog_opts::value<decltype(url)>(&url);
    auto url_desc = get_options_description(OPTIONS::URL).c_str();

    // creating our options table
    prog_opts::options_description desc(OPTIONS_DESCRIPTION);
    desc.add_options()(hlp_hdl, hlp_desc)(
        opt_hdl, opt_desc)(dev_hdl, dev_opt, dev_desc)(port_hdl, port_opt,
                                                       port_desc)(url_hdl, url_opt, url_desc);

    // grabbing options from command line
    prog_opts::variables_map vars_map;
    prog_opts::store(prog_opts::parse_command_line(argc, argv, desc), vars_map);
    prog_opts::notify(vars_map);

    // if user selected help option
    if (vars_map.count(get_options_long_handle(OPTIONS::HELP))) {
        print_version_number();
        std::cout << CAMSRV_APPLICATION_DESCRIPTION << std::endl;
        std::cout << desc << std::endl;
        return 0;
    }
    // if user selected version option
    else if (vars_map.count(get_options_long_handle(OPTIONS::VERSION))) {
        print_version_number();
        return 0;
    }

    // verifying usage is correct
    bool dev_set = !device_name.empty();
    bool port_set = vars_map.count(get_options_long_handle(OPTIONS::PORT));
    bool url_set = vars_map.count(get_options_long_handle(OPTIONS::URL));

    if (dev_set && url_set) {
        std::cout << "you can only select one or the other, device stream or url "
                     "stream, not both"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    } else if (!dev_set && !url_set) {
        std::cout << "no device node or url was selected" << std::endl;
        std::exit(EXIT_FAILURE);
    } else if (!port_set) {
        std::cout << "no port option was selected" << std::endl;
        std::exit(EXIT_FAILURE);
    } else {
        if ((port_number < MIN_PORT) || (port_number > MAX_PORT)) {
            std::cout << "port range must be within " << MIN_PORT << " - " << MAX_PORT << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    // creating our camsrv object
    std::make_unique<Controller>(device_name, port_number, url);

    return 0;
}
