cmake_minimum_required(VERSION 3.7.2)
project(thread_demo CXX)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(thread_demo main.cpp)
target_link_libraries(thread_demo
  PRIVATE Threads::Threads
  ${CONAN_LIBS}
)

add_subdirectory(tests)