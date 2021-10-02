cmake_minimum_required(VERSION 3.7.2)
project(compute VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

INCLUDE(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
CONAN_BASIC_SETUP()

include_directories(include)

add_subdirectory(src)
