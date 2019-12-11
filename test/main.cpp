#include "../cli++.h"
#include "../usage.h"
#include <cstdlib>

auto main() -> int {

    bool show_help;
    bool verbose;
    std::string filename;

    auto cl = cli::command{"usage header"};
    cl.options = {
        cli::option{show_help, "help", "h?", "display help info"},
        cli::option{verbose, "verbose", "v", "show detailed info"},
    };
    cl.arguments = {
        cli::argument{filename, "FILENAME", "load filename"},
    };

    auto s = cli::print_usage("TEST ", cl);
    printf(s.c_str());

    return 0;
}