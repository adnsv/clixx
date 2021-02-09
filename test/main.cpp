#include "../cli++.hpp"
#include <cstdlib>

auto main() -> int
{
    // declare storage for flags and arguments accessible by all commands and
    // subcommands
    std::optional<bool> verbose;
    std::string filename;

    auto cl = cli::app{"App Description"};

    cl.subcommand("info", "show information", [](cli::command& cmd) {
        // declare parameter storage accessible by "info" subcommand
        static std::optional<bool> detailed;
        cmd.flag(detailed, "", "d", "show detailed info");
        cmd.action = []() { printf("executing info command\n"); };
    });

    cl.flag(verbose, "verbose", "v", "show detailed info");
    cl.arg(filename, "FILENAME", "load filename");
    cl.action = []() { printf("executing root command\n"); };

    try {
        cl.execute({"EXENAME.exe", "info"});
    }
    catch (const cli::help& msg) {
        printf("%s\n", msg.what());
    }
    catch (const cli::error& msg) {
        printf("error: %s\n", msg.what());
    }

    return 0;
}