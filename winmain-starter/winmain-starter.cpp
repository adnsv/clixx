#include <string>
#include <vector>
#include <windows.h>

// bootstrap for windows GUI
extern auto main(int argc, char* argv[]) -> int;

auto WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
    -> int
{
    auto argv = std::vector<char*>{};
    auto u16 = static_cast<const wchar_t*>(::GetCommandLineW());
    auto n8 = ::WideCharToMultiByte(
        CP_UTF8, 0, u16, -1, nullptr, 0, nullptr, nullptr);
    auto buf = std::string{};
    buf.resize(n8);
    ::WideCharToMultiByte(
        CP_UTF8, 0, u16, -1, buf.data(), n8, nullptr, nullptr);

    auto i = 0;
    while (buf[i]) {
        while (isspace(buf[i]))
            ++i;

        if (buf[i]) {
            auto q = buf[i];
            if ((q == '\'') || (q == '"')) {
                ++i;
                if (!buf[i])
                    break;
                argv.push_back(&buf[i]);
                while (buf[i] && buf[i] != q)
                    ++i;
            }
            else {
                argv.push_back(&buf[i]);
                while (buf[i] && !isspace(buf[i]))
                    ++i;
            }
            if (buf[i] != 0)
                buf[i++] = 0;
        }
    }
    return main(int(argv.size()), argv.data());
}
