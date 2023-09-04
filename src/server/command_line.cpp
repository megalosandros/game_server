#include "../sdk.h"
#include <iostream>
#include <boost/program_options.hpp>

#include "command_line.h"

namespace util {

using namespace std::literals;

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[])
{
    namespace po = boost::program_options;

    Args args;

    po::options_description desc{"Allowed options"s};
    desc.add_options()                     //
        ("help,h", "produce help message") //
        ("config-file,c", po::value(&args.congig_file)->required()->value_name("file"s),
         "set config file path") //
        ("www-root,w", po::value(&args.www_root)->required()->value_name("dir"s),
         "set static files root") //
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s)->default_value(0, ""),
         "set tick period (optional)") //
        ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points),
         "spawn dogs at random positions (optional)")
         ("state-file", po::value(&args.state_file)->value_name("file"s),
         "set save configuration file (optional)")
         ("save-state-period", po::value(&args.save_state_period)->value_name("milliseconds"s)->default_value(0, ""),
         "set auto save configuration period (optional)"); //

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    //
    //  если просто просят хэлп - то не надо мапить параметры на значения
    //
    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }

    //
    //  если на заданы config-file и www-root - будет исключение
    //
    po::notify(vm);

    return args;
}


} // namespace util
