#pragma once

#include "cli++.h"

namespace cli {

struct writer {
    std::string buf;
    std::vector<std::vector<std::string>> rows;

    auto put(std::string_view sv) { buf += sv; }

    auto put_white(size_t n)
    {
        static constexpr auto sp16 = std::string_view{"                "};
        while (n >= 16) {
            put(sp16);
            --n;
        }
        put(sp16.substr(0, n));
    }

    auto line(std::string_view sv)
    {
        buf += sv;
        buf += "\n";
    }

    auto cols(std::initializer_list<std::string> cells)
    {
        rows.emplace_back(cells);
    }

    auto done_cols(std::string_view prefix = {}, std::string_view colsep = {})
    {
        auto ncols = size_t{0};
        for (auto& row : rows)
            ncols = std::max(ncols, row.size());
        std::vector<size_t> widths;
        widths.resize(ncols, 0);

        for (auto& row : rows) {
            auto col_index = 0;
            for (auto& cell : row) {
                widths[col_index] =
                    std::max(widths[col_index], calc_width(cell));
                ++col_index;
            }
        }

        for (auto& row : rows) {
            if (!row.empty()) {
                buf += prefix;
                for (size_t col_index = 0; col_index < row.size();
                     ++col_index) {
                    auto& cell = row[col_index];
                    buf += cell;
                    if (cell != row.back()) {
                        auto w = calc_width(cell);
                        put_white(widths[col_index] - w);
                        put(colsep);
                    }
                }
            }
            buf += '\n';
        }
        rows = {};
    }

private:
    auto calc_width(std::string_view sv) -> size_t
    {
        // todo: cell widths
        auto ret = size_t{0};
        for (auto c : sv) {
            if (c == '\n')
                ret = 0;
            else if (unsigned(c) < 32)
                continue;
            else if (unsigned(c) < 0b10000000)
                ++ret;
            else if (unsigned(c) < 0b11000000) // non-starter
                ret += 0;
            else if (unsigned(c) < 0b11100000)
                ret += 2;
            else if (unsigned(c) < 0b11110000)
                ret += 3;
            else if (unsigned(c) < 0b11111000)
                ret += 4;
        }
        return ret;
    }
};

inline auto syntax(const option& opt) -> std::string
{
    auto is_opt = false;
    auto is_string = false;
    auto is_array = false;
    if (std::holds_alternative<std::reference_wrapper<bool>>(opt.target)) {
    }
    else if (std::holds_alternative<
                 std::reference_wrapper<std::optional<bool>>>(opt.target)) {
        is_opt = true;
    }
    else if (std::holds_alternative<std::reference_wrapper<std::string>>(
                 opt.target)) {
        is_string = true;
    }
    else if (std::holds_alternative<
                 std::reference_wrapper<std::optional<std::string>>>(
                 opt.target)) {
        is_opt = true;
        is_string = true;
    }
    else if (std::holds_alternative<
                 std::reference_wrapper<std::vector<std::string>>>(
                 opt.target)) {
        is_string = true;
        is_array = true;
    }

    auto ret = std::string{};
    if (is_opt)
        ret += '[';

    if (!opt.name.empty())
        ret += std::string("--") + opt.name;
    else if (!opt.letters.empty())
        ret += std::string("-") + opt.letters[0];
    else
        ret = "#EMPTY#";
    if (is_string && !opt.sample.empty()) {
        ret += "=<";
        ret += opt.sample;
        ret += ">";
    }
    if (is_array) {
        ret += "...";
    }
    if (is_opt)
        ret += ']';
    return ret;
}

inline auto syntax(const option_list& ol) -> std::string
{
    if (ol.items.empty())
        return {};
    auto ret = std::string{};

    for (auto& it : ol.items) {
        auto s = std::string{};
        if (auto v = std::get_if<option_list>(&it)) {
            s = syntax(*v);
            if (v->exclusive) 
              s = std::string("(") + s + ")";
        }
        else if (auto v = std::get_if<option>(&it))
            s = syntax(*v);
        else
            s = "#OPTION#";
        if (!ret.empty()) 
            ret += ol.exclusive ? " | " : " ";
        
        ret += s;
    }
    return ret;
}

inline auto syntax(const argument& arg) -> std::string {
    auto ret = arg.name;
    if (std::holds_alternative<
                 std::reference_wrapper<std::optional<std::string>>>(
                 arg)) {
        ret = std::string("[") + ret + "]";
    }
    else if (std::holds_alternative<
                 std::reference_wrapper<std::vector<std::string>>>(
                 arg)) {
        ret += "..."
    }
    return ret;
}

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

inline auto print_usage(const std::string& prefix, const command& cmd) -> std::string
{
    auto w = writer{};

    if (!cmd.desc.empty())
    w.line(cmd.desc);

    auto s = prefix + syntax(cmd.options);
    if (!s.empty()) {
        w.line("\nsyntax:");
        w.line(std::string("    ") + s);
    }

    if (!cmd.subcommands.empty()) {
        w.line("\ncommands:");
        for (auto& sc : cmd.subcommands)
            w.cols({sc.first, sc.second.desc});
        w.done_cols("    ", "  ");
    }

    if (!cmd.options.items.empty()) {
        w.line("\noptions:");
        desc(w, cmd.options);
        w.done_cols("    ", "  ");
    }

    if (!cmd.arguments.empty()) {
        w.line("\narguments:");
        for (auto& arg : cmd.arguments)
            w.cols({arg.name, arg.desc});
        w.done_cols("    ", "  ");
    }

    w.line("");

    return w.buf;
}

} // namespace cli