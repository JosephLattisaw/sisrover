#include <iostream>
#include <boost/program_options.hpp>
#include "controller.hpp"

#define MIN_PORT 2000
#define MAX_PORT 65536

namespace
{
    const std::string CAMSRV_VERSION_NUMBER = "camsrv 4.0.0";
    const std::string CAMSRV_APPLICATION_DESCRIPTION = "camera server which feeds webcam images from remote cpu";

    const std::string OPTIONS_DESCRIPTION = "Options";
    const int OPTION_HANDLES_COMBO = 2;

    //OPTIONS
    const std::array<const std::array<std::string, 3>, 5> OPTIONS_HANDLE = {{
        {"help", "h", "Displays this help."},
        {"version", "v", "Displays version information"},
        {"device_name", "d", "Device name/path for a camera (e.g. /dev/video0)"},
        {"port", "p", "TCP/IP port for server to listen on."},
        {"url", "u", "URL for RTSP to IP Camera."},
    }};

    enum OPTIONS
    {
        HELP = 0,
        VERSION = 1,
        DEVICE_NAME = 2,
        PORT = 3,
        URL = 4,
    };

    enum OPTION_HANDLES
    {
        HANDLE = 0,
        SHORT_HANDLE = 1,
        DESCRIPTION = 2,
    };

    void print_version_number()
    {
        std::cout << CAMSRV_VERSION_NUMBER << std::endl;
    }

    std::string get_option_handles(OPTIONS index)
    {
        std::string res;

        try
        {
            auto opt_h = OPTIONS_HANDLE.at(index);
            res = opt_h.at(OPTION_HANDLES::HANDLE) + "," + opt_h.at(OPTION_HANDLES::SHORT_HANDLE);
        }
        catch (std::out_of_range const &exc)
        {
            std::cout << __PRETTY_FUNCTION__ << ", index: " << index << std::endl;
            throw;
        }

        return res;
    }

    std::string get_option_handle(OPTIONS options_index, OPTION_HANDLES handle_index)
    {
        std::string res;

        try
        {
            auto opt_h = OPTIONS_HANDLE.at(options_index);
            res = opt_h.at(handle_index);
        }
        catch (std::out_of_range const &exc)
        {
            std::cout << __PRETTY_FUNCTION__ << ", options_index: " << options_index << ", handle_index: " << handle_index << std::endl;
            throw;
        }

        return res;
    }

    std::string get_options_description(OPTIONS index)
    {
        return get_option_handle(index, OPTION_HANDLES::DESCRIPTION);
    }

    std::string get_options_long_handle(OPTIONS index)
    {
        return get_option_handle(index, OPTION_HANDLES::HANDLE);
    }
}

int main(int argc, char **argv)
{
    namespace prog_opts = boost::program_options;

    std::string device_name;
    std::uint16_t port_number;
    std::string url;

    prog_opts::options_description desc(OPTIONS_DESCRIPTION);
    desc.add_options()(get_option_handles(OPTIONS::HELP).c_str(), get_options_description(OPTIONS::HELP).c_str())(get_option_handles(OPTIONS::VERSION).c_str(), get_options_description(OPTIONS::VERSION).c_str())(get_option_handles(OPTIONS::DEVICE_NAME).c_str(), prog_opts::value<decltype(device_name)>(&device_name), get_options_description(OPTIONS::DEVICE_NAME).c_str())(get_option_handles(OPTIONS::PORT).c_str(), prog_opts::value<decltype(port_number)>(&port_number), get_options_description(OPTIONS::PORT).c_str())(get_option_handles(OPTIONS::URL).c_str(), prog_opts::value<decltype(url)>(&url), get_options_description(OPTIONS::URL).c_str());

    prog_opts::variables_map vars_map;
    prog_opts::store(prog_opts::parse_command_line(argc, argv, desc), vars_map);
    prog_opts::notify(vars_map);

    if (vars_map.count(get_options_long_handle(OPTIONS::HELP)))
    {
        print_version_number();
        std::cout << CAMSRV_APPLICATION_DESCRIPTION << std::endl;
        std::cout << desc << std::endl;
        return 0;
    }
    else if (vars_map.count(get_options_long_handle(OPTIONS::VERSION)))
    {
        print_version_number();
        return 0;
    }

    bool dev_set = !device_name.empty();
    bool port_set = vars_map.count(get_options_long_handle(OPTIONS::PORT));
    bool url_set = vars_map.count(get_options_long_handle(OPTIONS::URL));

    if (dev_set && url_set)
    {
        std::cout << "you can only select one or the other, device stream or url stream, not both" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    else if (!dev_set && !url_set)
    {
        std::cout << "no device node or url was selected" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    else if (!port_set)
    {
        std::cout << "no port option was selected" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    else
    {
        if ((port_number < MIN_PORT) || (port_number > MAX_PORT))
        {
            std::cout << "port range must be within " << MIN_PORT << " - " << MAX_PORT << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    std::make_unique<Controller>();

    return 0;
}
