#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace cli {

struct option {
    using target_type = std::variant<                //
        std::reference_wrapper<bool>,                // mandatory boolean flag
        std::reference_wrapper<std::optional<bool>>, // optional boolean flag
        std::reference_wrapper<std::string>,         // mandatory string
        std::reference_wrapper<std::optional<std::string>>, // optional string
        std::reference_wrapper<std::vector<std::string>>    // string list
        >;
    target_type target;
    std::string name;
    std::string letters;
    std::string sample;
    std::string desc;
    option(bool& target, const std::string& name, const std::string& letters,
        const std::string& desc)
        : target{target}
        , name{name}
        , letters{letters}
        , desc{desc}
    {
    }
    option(std::optional<bool>& target, const std::string& name,
        const std::string& letters, const std::string& desc)
        : target{target}
        , name{name}
        , letters{letters}
        , desc{desc}
    {
    }
    option(std::string& target, const std::string& name,
        const std::string& letters, const std::string& desc)
        : target{target}
        , name{name}
        , letters{letters}
        , desc{desc}
    {
    }
    option(std::optional<std::string>& target, const std::string& name,
        const std::string& letters, const std::string& desc)
        : target{target}
        , name{name}
        , letters{letters}
        , desc{desc}
    {
    }
    option(std::vector<std::string>& target, const std::string& name,
        const std::string& letters, const std::string& desc)
        : target{target}
        , name{name}
        , letters{letters}
        , desc{desc}
    {
    }
};

struct option_list {
    using item = std::variant<option, option_list>;
    std::vector<item> items;
    bool exclusive = false;
    option_list() = default;
    option_list(const option_list&) = default;
    auto operator=(const option_list&) -> option_list& = default;
    auto operator=(std::initializer_list<item> new_items) -> option_list
    {
        items = new_items;
        return *this;
    }
};

struct argument {
    using target_type = std::variant<        //
        std::reference_wrapper<std::string>, // mandatory argument
        std::reference_wrapper<std::optional<std::string>>, // optional argument
        std::reference_wrapper<std::vector<std::string>> // string list argument
        >;
    target_type target;
    std::string name;
    std::string desc;
    argument(
        std::string& target, const std::string& name, const std::string& desc)
        : target{target}
        , name{name}
        , desc{desc}
    {
    }
    argument(std::optional<std::string>& target, const std::string& name,
        const std::string& desc)
        : target{target}
        , name{name}
        , desc{desc}
    {
    }
    argument(std::vector<std::string>& target, const std::string& name,
        const std::string& desc)
        : target{target}
        , name{name}
        , desc{desc}
    {
    }
};

class command {
public:
    std::string desc;
    option_list options;
    std::vector<argument> arguments;
    std::map<std::string, command> subcommands;
    command(
        const std::string& desc)
        : desc{desc}
    {
    }
    auto print_usage() const -> std::string;
};



} // namespace cli