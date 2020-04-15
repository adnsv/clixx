cmake_minimum_required(VERSION 3.15)

project (cli++)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_library (cli++ INTERFACE)

target_include_directories (cli++ INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    option (BUILD_CLIXX_WINMAIN_STARTER "Build cli++ test" ON)
    if (BUILD_CLIXX_WINMAIN_STARTER)
        add_subdirectory ("internal/winmain-starter")
    endif (BUILD_CLIXX_WINMAIN_STARTER)
endif (WIN32)

option (BUILD_CLIXX_TEST "Build cli++ test" ON)
if (BUILD_CLIXX_TEST)
    add_subdirectory ("test")
endif (BUILD_CLIXX_TEST)