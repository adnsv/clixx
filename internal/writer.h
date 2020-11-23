#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace cli {

struct writer {
    std::string buf;
    std::vector<std::vector<std::string>> rows;

    auto put(std::string_view sv) { buf += sv; }

    auto put_white(size_t n)
    {
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

    auto cols(std::initializer_list<std::string_view> cells)
    {
        rows.push_back({cells.begin(), cells.end()});
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
    inline static std::string_view sp16 = "                ";

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

} // namespace cli