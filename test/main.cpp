#include "../cli++.h"
#include <cstdlib>

auto main() -> int
{
    std::optional<bool> show_help;
    std::optional<bool> verbose;
    std::string filename;

    auto cl = cli::app{"App Description"};
    cl.options = {
        cli::option{show_help, "help", "h?", "display help info"},
        cli::option{verbose, "verbose", "v", "show detailed info"},
    };
    cl.arguments = {
        cli::argument{filename, "FILENAME", "load filename"},
    };
    cl.subcommand("info", "show information", [](cli::command& cmd) {
        std::optional<bool> detailed;
        cmd.options = {
            cli::option{detailed, "", "d", "show detailed info"},
        };
    });

    auto s = cl.usage("EXENAME");
    printf(s.c_str());

    return 0;
}