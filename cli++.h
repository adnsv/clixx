#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "writer.h"

namespace cli {

class command;

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

    auto syntax() -> std::string
    {
        auto is_opt = false;
        auto is_string = false;
        auto is_array = false;
        if (std::holds_alternative<std::reference_wrapper<bool>>(target)) {
        }
        else if (std::holds_alternative<
                     std::reference_wrapper<std::optional<bool>>>(target)) {
            is_opt = true;
        }
        else if (std::holds_alternative<std::reference_wrapper<std::string>>(
                     target)) {
            is_string = true;
        }
        else if (std::holds_alternative<
                     std::reference_wrapper<std::optional<std::string>>>(
                     target)) {
            is_opt = true;
            is_string = true;
        }
        else if (std::holds_alternative<
                     std::reference_wrapper<std::vector<std::string>>>(
                     target)) {
            is_string = true;
            is_array = true;
        }

        auto ret = std::string{};
        if (is_opt)
            ret += '[';

        if (!name.empty())
            ret += std::string("--") + name;
        else if (!letters.empty())
            ret += std::string("-") + letters[0];
        else
            ret = "#EMPTY#";
        if (is_string && !sample.empty()) {
            ret += "=<";
            ret += sample;
            ret += ">";
        }
        if (is_array) {
            ret += "...";
        }
        if (is_opt)
            ret += ']';
        return ret;
    }

protected:
    auto name_match(std::string_view s, bool as_letter) -> bool
    {
        if (!as_letter)
            return s == name;
        else
            return s.size() == 1 && letters.find(s[0]) != std::string::npos;
    }
    friend class command;
    friend class option_list;
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

    auto syntax() -> std::vector<std::string>
    {
        auto ret = std::vector<std::string>{};
        for (auto& it : items) {
            auto s = std::string{};
            if (auto v = std::get_if<option_list>(&it)) {
                auto ss = v->syntax();
                if ((exclusive || v->exclusive) && ss.size() > 1) {
                    ss.front() = std::string("(") + ss.front();
                    ss.back() += ')';
                }
                ret.insert(ret.end(), ss.begin(), ss.end());
            }
            else if (auto v = std::get_if<option>(&it))
                ret.push_back(v->syntax());
        }
        if (exclusive && ret.size() > 0) {
            for (auto it = ret.begin() + 1; it != ret.end(); ++it)
                *it = std::string("| ") + *it;
        }

        return ret;
    }

protected:
    auto find_opt(std::string_view s, bool as_letter) -> option*
    {
        for (auto it : items) {
            if (auto v = std::get_if<option_list>(&it)) {
                auto opt = v->find_opt(s, as_letter);
                if (opt)
                    return opt;
            }
            else if (auto v = std::get_if<option>(&it)) {
                if (v->name_match(s, as_letter))
                    return v;
            }
        }
    }
    friend class command;
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

    auto syntax() const -> std::string
    {
        auto ret = name;
        if (std::holds_alternative<
                std::reference_wrapper<std::optional<std::string>>>(target)) {
            ret = std::string("[") + ret + "]";
        }
        else if (std::holds_alternative<
                     std::reference_wrapper<std::vector<std::string>>>(
                     target)) {
            ret += "...";
        }
        return ret;
    }
};

// command defines a boolean flag or a string option
class command {
public:
    using subcmd_callback = std::function<void(command& cmd)>;

    option_list options;
    std::vector<argument> arguments;

    auto subcommand(const std::string& name, const std::string& desc,
        subcmd_callback callback)
    {
        subcommands.push_back(subcmd{name, desc, callback});
    }

    // print command usage to a string
    auto usage(const std::string& syntax_prefix) -> std::string;

protected:
    struct subcmd {
        std::string name;
        std::string desc;
        subcmd_callback callback;
    };

    std::vector<subcmd> subcommands;
    command* parent_cmd = nullptr; // link to a parent command

    void exec(const std::string_view* first, const std::string_view* last);

    auto find_opt(std::string_view s, bool as_letter) -> option*
    {
        // find an option in this command or in one of its parent commands
        auto cmd = this;
        while (cmd) {
            auto ret = cmd->options.find_opt(s, as_letter);
            if (ret)
                return ret;
            cmd = cmd->parent_cmd;
        }
    }

    auto validate_opts(const std::vector<option*>& opts) const -> bool;
    auto collect_arguments(const std::vector<std::string_view>& args) const
        -> bool;
};

class app : public command {
public:
    std::string executable;
    std::string desc;

    app(const std::string& desc)
        : desc(desc)
    {
    }

    void execute(const std::string_view* first, const std::string_view* last);

    void execute(int argc, char* argv[])
    {
        auto args = std::vector<std::string_view>{};
        args.reserve(argc);
        for (auto i = 0; i < argc; ++i)
            args.push_back(argv[i]);
        execute(args.data(), args.data() + args.size());
    }

    void execute(int argc, wchar_t* argv[])
    {
        auto strings = std::vector<std::string>{};
        auto args = std::vector<std::string_view>{};
        args.reserve(argc);
        strings.reserve(argc);
        for (auto i = 0; i < argc; ++i) {
            // use filesystem::path for wide_string->string conversion
            // until something more appropriate is available
            auto conv = std::filesystem::path{argv[i]};
            strings.push_back(conv.string());
            args.push_back(strings.back());
        }
        execute(args.data(), args.data() + args.size());
    }
};

namespace {

inline void desc(writer& w, const option& opt)
{
    auto s = std::string{};
    if (!opt.name.empty())
        s = std::string("--") + opt.name;
    for (auto& ltr : opt.letters) {
        if (!s.empty())
            s += ' ';
        s += '-';
        s += ltr;
    }
    w.cols({s, opt.desc});
}

inline void desc(writer& w, const option_list& ol)
{
    for (auto& it : ol.items)
        if (auto v = std::get_if<option_list>(&it))
            desc(w, *v);
        else if (auto v = std::get_if<option>(&it))
            desc(w, *v);
}

} // namespace

inline auto command::usage(const std::string& syntax_prefix) -> std::string
{
    auto w = writer{};

    auto parts = std::vector<std::string>{};
    if (!subcommands.empty()) {
        parts.push_back("command");
    }
    for (auto& prt : options.syntax())
        parts.push_back(prt);
    for (auto& arg : arguments)
        parts.push_back(arg.syntax());

    auto s = syntax_prefix;
    for (auto& part : parts)
        s += " " + part;
    w.line("\nsyntax:");
    w.line(std::string("    ") + s);

    if (!subcommands.empty()) {
        w.line("\ncommands:");
        for (auto& sc : subcommands)
            w.cols({sc.name, sc.desc});
        w.done_cols("    ", "  ");
    }

    if (!options.items.empty()) {
        w.line("\noptions:");
        desc(w, options);
        w.done_cols("    ", "  ");
    }

    if (!arguments.empty()) {
        w.line("\narguments:");
        for (auto& arg : arguments)
            w.cols({arg.name, arg.desc});
        w.done_cols("    ", "  ");
    }

    w.line("");

    return w.buf;
}

inline void command::exec(
    const std::string_view* first, const std::string_view* last)
{
    // arg_strings collects all free-standing arguments
    auto arg_strings = std::vector<std::string_view>{};

    // processed_options collects all options that were found
    auto processed_options = std::vector<option*>{};

    auto is_processed = [&](option* opt) -> bool {
        return std::find(processed_options.begin(), processed_options.end(),
                   opt) != processed_options.end();
    };

    if (first != last) {
        // do we have a command?
        for (auto& sub : subcommands) {
            if (sub.name == *first) {
                auto cmd = command{};
                cmd.parent_cmd = this;
                if (sub.callback)
                    sub.callback(cmd);
                cmd.exec(first + 1, last);
                return;
            }
        }
    }

    while (first != last) {
        // parse the rest of the args
        auto& sv = *first++;

        auto eqpos = sv.find('=');
        option* opt = nullptr;
        auto opt_as_text = std::string{};   // for error reporting
        auto foldings = std::string_view{}; // -abcd -> bcd part

        if (sv.size() > 2 && sv[0] == '-' && sv[1] == '-') {
            // long name option
            opt = find_opt(sv.substr(2, eqpos), false);
            opt_as_text = sv;
            if (!opt)
                throw error{std::string{"unsupported option "} + opt_as_text};
        }
        else if (sv.size() > 1 && sv[0] == '-') {
            // a single flag or a folding of boolean flags
            opt = find_opt(sv.substr(1, 2), true); // first one in a sequence
            opt_as_text = sv.substr(0, 2);
            if (!opt)
                throw error{std::string{"unsupported option -"} + opt_as_text};
            foldings = sv.substr(2, eqpos); // the rest of them foldings, if any
        }

        if (opt) {
            // do we have '=optval' part ?
            auto optval = std::optional<std::string_view>{};
            auto optval_as_bool = std::optional<bool>{};
            if (eqpos != std::string_view::npos) {
                optval = sv.substr(eqpos + 1);
                if (optval->size() >= 2 && optval->front() == '"' &&
                    optval->back() == '"') {
                    optval = optval->substr(1, optval->size() - 1);
                }
                else {
                    // is it '=true' or '=false'?
                    if (optval == "true")
                        optval_as_bool = true;
                    else if (optval == "false")
                        optval_as_bool = false;
                }
            }

            if (auto v =
                    std::get_if<std::reference_wrapper<bool>>(&opt->target)) {
                // mandatory bool flag
                if (is_processed(opt))
                    throw error{"duplicate option " + opt_as_text};
                processed_options.push_back(opt);
                v->get() = optval_as_bool.value_or(true);
            }
            else if (auto v = std::get_if<
                         std::reference_wrapper<std::optional<bool>>>(
                         &opt->target)) {
                // optional bool flag
                if (is_processed(opt))
                    throw error{"duplicate option " + opt_as_text};
                processed_options.push_back(opt);
                v->get() = optval_as_bool.value_or(true);
            }
            else if (auto v = std::get_if<std::reference_wrapper<std::string>>(
                         &opt->target)) {
                // mandatory string option
                if (!foldings.empty())
                    throw error{"unsupported folding on non-boolean flags"};
                if (is_processed(opt))
                    throw error{"duplicate option " + opt_as_text};
                processed_options.push_back(opt);
                if (!optval) {
                    // read space-separated value
                    if (first != last)
                        optval = *first++;
                    else
                        throw error{std::string("missing parameter for flag ") +
                                    std::string(sv)};
                }
                v->get() = *optval;
            }
            else if (auto v = std::get_if<
                         std::reference_wrapper<std::optional<std::string>>>(
                         &opt->target)) {
                // optional string option
                if (!foldings.empty())
                    throw error{"unsupported folding on non-boolean flags"};
                if (is_processed(opt))
                    throw error{"duplicate option " + opt_as_text};
                processed_options.push_back(opt);
                if (!optval) {
                    // read space-separated value
                    if (first != last)
                        optval = *first++;
                    else
                        throw error{std::string{"missing parameter for flag "} +
                                    std::string{sv}};
                }
                v->get() = *optval;
            }
            else if (auto v = std::get_if<
                         std::reference_wrapper<std::vector<std::string>>>(
                         &opt->target)) {
                // string-list option
                if (!foldings.empty())
                    throw error{"unsupported folding on non-boolean flags"};

                if (!optval) {
                    // read space-separated value
                    if (first != last)
                        optval = *first++;
                    else
                        throw error{std::string("missing parameter for flag ") +
                                    std::string(sv)};
                }
                v->get().push_back(std::string{*optval});
            }

            if (!foldings.empty()) {
                // process the remaining 'b', 'c', 'd' parts in the '-abcd'
                // boolean folding if we are here, then the first letter in a
                // folding is validated to be a boolean flag
                for (auto& f : foldings) {
                    auto opt = find_opt({&f, 1}, true);
                    if (!opt)
                        throw error{std::string{"unknown flag -"} + f};
                    if (auto v = std::get_if<std::reference_wrapper<bool>>(
                            &opt->target)) {
                        v->get() = optval_as_bool.value_or(true);
                    }
                    else if (auto v = std::get_if<
                                 std::reference_wrapper<std::optional<bool>>>(
                                 &opt->target)) {
                        v->get() = optval_as_bool.value_or(true);
                    }
                    else
                        throw error{std::string{"unsupported folding on "
                                                "non-boolean flag -"} +
                                    f};
                }
            }
        }
        else {
            // free standing, non-option and non-flag argument
            arg_strings.push_back(sv);
        }
    }
}

auto command::validate_opts(const std::vector<option*>& opts) const -> bool {}

auto command::collect_arguments(const std::vector<std::string_view>& args) const
    -> bool
{
}

inline void app::execute(
    const std::string_view* first, const std::string_view* last)
{
    if (first != last)
        executable = *first++;
    exec(first, last);
}

} // namespace cli