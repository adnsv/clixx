cmake_minimum_required(VERSION 3.15)

project (cli++)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_library (cli++ INTERFACE)

target_include_directories (cli++ INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    option (CLIXX_BUILD_WINMAIN_STARTER "Build cli++ WinMain starter" ON)
    if (CLIXX_BUILD_WINMAIN_STARTER)
        add_library (winmain-starter STATIC "winmain-starter/winmain-starter.cpp")
    endif (CLIXX_BUILD_WINMAIN_STARTER)
endif (WIN32)

option (CLIXX_BUILD_TEST "Build cli++ test" ON)
if (CLIXX_BUILD_TEST)
    add_subdirectory ("test")
endif (CLIXX_BUILD_TEST)