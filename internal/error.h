#pragma once

#include <stdexcept>

namespace cli {

// cli::error is the exception that may be thrown while
// processing command line arguments
struct error : public std::domain_error {
    explicit error(const std::string& what_arg)
        : std::domain_error(what_arg.c_str())
    {
    }
    explicit error(const char* what_arg)
        : std::domain_error(what_arg)
    {
    }
};

struct help : public error {
    explicit help(const std::string& what_arg)
        : error(what_arg.c_str())
    {
    }
};

} // namespace cli