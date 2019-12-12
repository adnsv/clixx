#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "internal/error.h"
#include "internal/target.h"
#include "internal/writer.h"

namespace cli {

class command;

struct flag {
    target t;
    std::string name;
    std::string letters;
    std::string sample;
    std::string desc;

    flag(const target& t, const std::string& name, const std::string& letters,
        const std::string& desc, const std::string& sample = {})
        : t{t}
        , name{name}
        , letters{letters}
        , desc{desc}
        , sample{sample}
    {
    }

    auto syntax(bool show_samples = true) const -> std::string
    {
        auto ret = std::string{};
        ret += printable_name();
        if (show_samples && !sample.empty()) {
            ret += "=<";
            ret += sample;
            ret += ">";
        }
        if (t.is_vector())
            ret += "...";
        return ret;
    }

    auto printable_name(bool prefer_long = true) const -> std::string
    {

        if (!name.empty() && (prefer_long || letters.empty()))
            return std::string("--") + name;
        if (!letters.empty())
            return std::string("-") + letters[0];
        else
            return "#EMPTY#";
    }

    auto used() const -> bool { return !used_as.empty(); }

protected:
    std::string used_as;

    auto name_match(std::string_view s, bool as_letter) -> bool
    {
        if (!as_letter)
            return s == name;
        else
            return s.size() == 1 && letters.find(s[0]) != std::string::npos;
    }
    friend class command;
    friend class flag_list;
};

struct flag_list {
    using item = std::variant<flag, flag_list>;
    std::vector<item> items;
    bool required = false;  // at lease one of the flags has to be used
    bool exclusive = false; // at most one of the flags has to be used

    flag_list() = default;
    flag_list(const flag_list&) = default;

    auto operator=(const flag_list&) -> flag_list& = default;
    auto operator=(std::initializer_list<item> new_items) -> flag_list
    {
        items = new_items;
        return *this;
    }

    auto syntax(bool show_samples = true) const -> std::vector<std::string>;

protected:
    auto find(std::string_view s, bool as_letter) -> flag*;
    auto validate() -> bool;
    friend class command;
};

// defines a stand-alone argument (not a flag)
struct argument {
    target t;
    std::string name;
    std::string desc;

    auto syntax() const -> std::string
    {
        auto ret = name;
        if (t.is_vector())
            ret += "...";
        if (!t.required)
            ret = std::string("[") + ret + "]";
        return ret;
    }

protected:
    argument(const target& t, const std::string& name, const std::string& desc)
        : t{t}
        , name{name}
        , desc{desc}
    {
    }

    friend class command;
};

// command defines a set of flags and arguments
class command {
public:
    using subcmd_callback = std::function<void(command& cmd)>;

    flag_list flags;
    std::vector<argument> arguments;
    std::function<void()> action;

    auto subcommand(const std::string& name, const std::string& desc,
        subcmd_callback callback)
    {
        subcommands.push_back(subcmd{name, desc, callback});
    }

    void flag(const target& t, const std::string& name,
        const std::string& letters, const std::string& desc,
        const std::string& sample = {})
    {
        flags.items.push_back(cli::flag{t, name, letters, desc, sample});
    }

    // todo: add function that constructs flag lists

    void arg(const target& t, const std::string& name, const std::string& desc)
    {
        arguments.push_back(argument{t, name, desc});
    }

protected:
    struct subcmd {
        std::string name;
        std::string desc;
        subcmd_callback callback;
    };

    std::vector<subcmd> subcommands;
    command* parent_cmd = nullptr; // link to a parent command

    void exec(const std::string_view* first, const std::string_view* last);

    auto find_flag(std::string_view s, bool as_letter) -> cli::flag*
    {
        // find a flag in this command or in one of its parent commands
        auto cmd = this;
        while (cmd) {
            auto ret = cmd->flags.find(s, as_letter);
            if (ret)
                return ret;
            cmd = cmd->parent_cmd;
        }
        return nullptr;
    }

    auto trace_usage(                  //
        const std::string& exe_prefix, // executable command name
        const std::string& cmd_prefix, // parent command chain (if any)
        const std::string& help_cmd,   // a command (or flag) for usage help
        const std::string_view* first, // command line arguments
        const std::string_view* last) -> std::string;

    auto usage(const std::string& exe_prefix, const std::string& cmd_prefix,
        const std::string& help_cmd) const -> std::string;

    void collect_arguments(
        const std::string_view* first, const std::string_view* last);
};

// app is the root command
class app : public command {
public:
    std::string name;
    std::string desc;
    std::string help_cmd = {};
    std::string help_flag = "--help";

    app(const std::string& name, const std::string& desc)
        : name{name}
        , desc{desc}
    {
    }

    void execute(const std::string_view* first, const std::string_view* last);

    void execute(std::initializer_list<std::string_view> cmdline)
    {
        execute(cmdline.begin(), cmdline.end());
    }

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

protected:
    std::filesystem::path
        executable_path; // obtained from the first command line parameter
};

namespace {

inline auto wrap_brackets(const std::string& s) -> std::string
{
    return std::string{"["} + s + "]";
}

inline auto wrap_parenthesis(const std::string& s) -> std::string
{
    return std::string{"("} + s + ")";
}

inline void desc(writer& w, const flag& f)
{
    auto s = std::string{};
    if (!f.name.empty())
        s = std::string("--") + f.name;
    for (auto& ltr : f.letters) {
        if (!s.empty())
            s += ' ';
        s += '-';
        s += ltr;
    }
    w.cols({s, f.desc});
}

inline void desc(writer& w, const flag_list& ol)
{
    for (auto& it : ol.items)
        if (auto v = std::get_if<flag_list>(&it))
            desc(w, *v);
        else if (auto v = std::get_if<flag>(&it))
            desc(w, *v);
}

} // namespace

inline auto flag_list::syntax(bool show_samples) const
    -> std::vector<std::string>
{
    auto ret = std::vector<std::string>{};
    for (auto& it : items) {
        auto s = std::string{};
        if (auto fl = std::get_if<flag_list>(&it)) {
            auto ss = fl->syntax(show_samples);

            if (this->exclusive) {
                if (ss.size() > 1) {
                    ss.front() = std::string{"("} + ss.front();
                    ss.back() += ')';
                }
            }
            else if (fl->required) {
                if (ss.size() > 1) {
                    ss.front() = std::string{"("} + ss.front();
                    ss.back() += ')';
                }
            }
            else if (!ss.empty()) {
                ss.front() = std::string{"["} + ss.front();
                ss.back() += ']';
            }

            ret.insert(ret.end(), ss.begin(), ss.end());
        }
        else if (auto f = std::get_if<flag>(&it)) {
            auto s = f->syntax(show_samples);
            auto show_as_required = f->t.required && !this->exclusive;
            if (!show_as_required)
                s = wrap_brackets(s);
            ret.push_back(s);
        }
    }
    if (exclusive && ret.size() > 1)
        for (auto it = ret.begin() + 1; it != ret.end(); ++it)
            *it = std::string("| ") + *it;

    return ret;
}

inline auto flag_list::find(std::string_view s, bool as_letter) -> flag*
{
    for (auto it : items) {
        if (auto fl = std::get_if<flag_list>(&it)) {
            if (auto f = fl->find(s, as_letter))
                return f;
        }
        else if (auto f = std::get_if<flag>(&it)) {
            if (f->name_match(s, as_letter))
                return f;
        }
    }
    return nullptr;
}

inline auto flag_list::validate() -> bool
{
    // returns true any of the contained flags is in use
    // throws when required flags are missing, when
    // exclusivity is violated, and other logical error.
    auto unused_but_required = std::vector<const item*>{};
    auto used = std::vector<const item*>{};

    for (auto it : items) {
        if (auto fl = std::get_if<flag_list>(&it)) {
            if (fl->validate())
                used.push_back(&it);
        }
        else if (auto f = std::get_if<flag>(&it)) {
            if (f->used())
                used.push_back(&it);
            else if (f->t.required)
                unused_but_required.push_back(&it);
        }
    }

    auto as_str = [](const item* it) -> std::string {
        auto ret = std::string{};
        if (auto fl = std::get_if<flag_list>(it)) {
            for (auto& s : fl->syntax(false)) {
                ret += ' ';
                ret += s;
            }
            if (fl->exclusive)
                ret = wrap_parenthesis(ret);
        }
        else if (auto f = std::get_if<flag>(it)) {
            ret += ' ';
            ret += f->syntax(false);
        }
        return ret;
    };

    if (unused_but_required.size() > 1) {
        auto err_msg = std::string{"missing required flags:"};
        for (auto& it : unused_but_required)
            err_msg += as_str(it);
        throw error{err_msg};
    }
    if (exclusive && used.size() > 1) {
        auto err_msg = std::string{"use only one of the folloging flags:"};
        for (auto& it : used)
            err_msg += as_str(it);
        throw error{err_msg};
    }
    if (required && used.empty() && !items.empty()) {
        auto err_msg = std::string{"use at least one of the following flags:"};
        for (auto& it : items)
            err_msg += as_str(&it);
        throw error{err_msg};
    }

    return !used.empty();
}

inline auto command::usage(const std::string& exe_prefix,
    const std::string& cmd_prefix, const std::string& help_cmd) const
    -> std::string
{
    auto w = writer{};

    auto parts = std::vector<std::string>{};
    if (!subcommands.empty()) {
        parts.push_back("<command>");
    }
    for (auto& prt : flags.syntax())
        parts.push_back(prt);
    for (auto& arg : arguments)
        parts.push_back(arg.syntax());

    auto s = exe_prefix;
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

    if (!flags.items.empty()) {
        w.line("\nflags:");
        desc(w, flags);
        w.done_cols("    ", "  ");
    }

    auto cmd = parent_cmd;
    while (cmd && cmd->flags.items.empty())
        cmd = cmd->parent_cmd;
    if (cmd) {
        w.line("\nparent flags:");
        while (cmd) {
            if (!cmd->flags.items.empty())
                desc(w, cmd->flags);
            cmd = cmd->parent_cmd;
        }
        w.done_cols("    ", "  ");
    }

    if (!arguments.empty()) {
        w.line("\narguments:");
        for (auto& arg : arguments)
            w.cols({arg.name, arg.desc});
        w.done_cols("    ", "  ");
    }

    if (!subcommands.empty() && !help_cmd.empty()) {
        w.line("\nuse '" + exe_prefix + help_cmd + cmd_prefix +
               " <command>' for more information about a command.");
    }

    w.line("");

    return w.buf;
}

inline void command::exec(
    const std::string_view* first, const std::string_view* last)
{
    // arg_strings collects all free-standing arguments
    auto arg_strings = std::vector<std::string_view>{};

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
        cli::flag* f = nullptr;
        auto used_as = std::string{};       // for error reporting
        auto foldings = std::string_view{}; // -abcd -> bcd part

        if (sv.size() > 2 && sv[0] == '-' && sv[1] == '-') {
            // long name flag
            f = find_flag(sv.substr(2, eqpos), false);
            used_as = sv;
            if (!f)
                throw error{std::string{"unsupported flag "} + used_as};
        }
        else if (sv.size() > 1 && sv[0] == '-') {
            // a single flag or a folding of boolean flags
            f = find_flag(sv.substr(1, 2), true); // first one in a sequence
            used_as = sv.substr(0, 2);
            if (!f)
                throw error{std::string{"unsupported flag "} + used_as};
            foldings = sv.substr(2, eqpos); // the rest of them foldings, if any
        }

        if (f) {
            if (!f->t.is_bool()) {
                // string flag
                if (!foldings.empty())
                    throw error{
                        std::string(
                            "unsupported folding on non-boolean flag ") +
                        used_as};
                auto value = std::string_view{};
                if (eqpos != std::string_view::npos) {
                    // handle: --flag=value
                    value = sv.substr(eqpos + 1);
                    if (value.size() >= 2 && value.front() == '"' &&
                        value.back() == '"') {
                        // unquote: --flag="value"
                        value = value.substr(1, value.size() - 1);
                    }
                }
                else {
                    // handle space separated: --flag value
                    if (first != last)
                        value = *first++;
                    else
                        throw error{
                            std::string(
                                "missing required parameter for flag ") +
                            used_as};
                }
                if (f->used() && !f->t.is_vector())
                    throw error{"duplicate flag " + used_as};
                f->t.write(value);
                f->used_as = used_as;
            }
            else {
                // boolean flag (or folding)
                auto value = true;
                if (eqpos != std::string_view::npos) {
                    // handle: --flag=true or --flag=false
                    if (sv.substr(eqpos + 1) == "true")
                        value = true;
                    else if (sv.substr(eqpos + 1) == "false")
                        value = false;
                    else
                        throw error{
                            "unsupported boolean value for flag " + used_as};
                }
                if (f->used())
                    throw error{"duplicate flag " + used_as};
                f->t.write(value);
                f->used_as = used_as;

                // process the remaining 'b', 'c', 'd' parts in the '-abcd'
                // boolean folding if we are here, then the first letter in a
                // folding is validated to be a boolean flag
                for (auto& c : foldings) {
                    auto f = find_flag({&c, 1}, true);
                    used_as = std::string{"-"} + c;
                    if (!f)
                        throw error{std::string{"unknown flag "} + used_as};
                    if (!f->t.is_bool())
                        throw error{std::string{"unsupported folding on "
                                                "non-boolean flag "} +
                                    used_as};
                    if (!f->used_as.empty())
                        throw error{"duplicate flag " + used_as};
                    f->t.write(value);
                    f->used_as = used_as;
                }
            }
        }
        else {
            // free standing, non-flag argument
            arg_strings.push_back(sv);
        }
    }
    auto cmd = this;
    while (cmd) {
        cmd->flags.validate();
        cmd = cmd->parent_cmd;
    }

    collect_arguments(
        arg_strings.data(), arg_strings.data() + arg_strings.size());

    if (action)
        action();
}

void command::collect_arguments(
    const std::string_view* first, const std::string_view* last)
{
    if (arguments.empty()) {
        if (first != last)
            throw error{
                std::string{"unexpected argument: "} + std::string{*first}};
        return;
    }
    auto b = arguments.begin();
    auto e = arguments.end();

    while (b != e && b->t.required && !b->t.is_vector()) {
        if (first == last)
            throw error{std::string{"missing argument: "} + b->name};
        b->t.write(*first);
        ++b;
        ++first;
    }
    while (b != e && (e - 1)->t.required && !(e - 1)->t.is_vector()) {
        if (first == last)
            throw error{std::string{"missing argument: "} + b->name};
        --e;
        --last;
        b->t.write(*last);
    }
    while (b != e && !b->t.required && !b->t.is_vector()) {
        if (first != last) {
            b->t.write(*first);
            ++b;
        }
        ++first;
    }
    if (b != e && b->t.is_vector()) {
        if (b->t.required && first == last)
            throw error{std::string{"missing argument: "} + b->name};
        while (first != last) {
            b->t.write(*first);
            ++first;
        }
        ++b;
    }
    if (b != e)
        throw error{"invalid argument declaration"};
    if (first != last)
        throw error{std::string{"unexpected argument: "} + std::string{*first}};
}

inline auto command::trace_usage(  //
    const std::string& exe_prefix, // executable command name
    const std::string& cmd_prefix, // parent command chain (if any)
    const std::string& help_cmd,   // a command (or flag) for usage help
    const std::string_view* first, // command line arguments
    const std::string_view* last) -> std::string
{
    if (first != last)
        // do we have a subcommand?
        for (auto& sub : subcommands)
            if (sub.name == *first) {
                auto cmd = command{};
                cmd.parent_cmd = this;
                if (sub.callback)
                    sub.callback(cmd);
                return cmd.trace_usage(exe_prefix, cmd_prefix + " " + sub.name,
                    help_cmd, first + 1, last);
            }

    return usage(exe_prefix, cmd_prefix, help_cmd);
}

inline void app::execute(
    const std::string_view* first, const std::string_view* last)
{
    if (first != last) {
        executable_path = *first++;
        if (name.empty())
            name = executable_path.filename().replace_extension("").string();
    }

    // check for help:
    auto show_help = false;
    if (first != last) {
        if (!help_cmd.empty() && *first == help_cmd) {
            show_help = true;
            ++first;
        }
        else if (!help_flag.empty() && *first == help_flag) {
            show_help = true;
            ++first;
        }
    }

    if (show_help) {
        auto msg = desc;
        if (!msg.empty())
            msg += '\n';
        auto h = help_cmd;
        if (h.empty()) h = help_flag;
        msg += trace_usage(name, "", h, first, last);
        throw help{msg};
    }

    exec(first, last);
}

} // namespace cli