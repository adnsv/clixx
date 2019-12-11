cmake_minimum_required(VERSION 3.15)

project (cli++test)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable (cli++test main.cpp)

target_link_libraries (cli++test cli++)