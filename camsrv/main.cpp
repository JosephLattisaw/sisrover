#include <iostream>
#include <boost/program_options.hpp>

const std::string CAMSRV_VERSION_NUMBER = "camsrv 4.0.0";
const std::string CAMSRV_APPLICATION_DESCRIPTION = "camera server which feeds webcam images from remote cpu";

void print_version_number()
{
    std::cout << CAMSRV_VERSION_NUMBER << std::endl;
}

int main(int argc, char **argv)
{
    namespace prog_opts = boost::program_options;

    prog_opts::options_description desc("Options");
    desc.add_options()("help,h", "Displays this help.")("version,v", "Displays version information.");

    prog_opts::variables_map vars_map;
    prog_opts::store(prog_opts::parse_command_line(argc, argv, desc), vars_map);
    prog_opts::notify(vars_map);

    if (vars_map.count("help"))
    {
        print_version_number();
        std::cout << CAMSRV_APPLICATION_DESCRIPTION << std::endl;
        std::cout << desc << std::endl;
        return 0;
    }
    else if (vars_map.count("version"))
    {
        print_version_number();
        return 0;
    }
}
