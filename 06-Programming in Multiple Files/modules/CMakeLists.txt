cmake_minimum_required(VERSION 3.29 FATAL_ERROR)
set(ENV{CXXFLAGS} "-stdlib=libc++")
set(CMAKE_CXX_COMPILER /usr/bin/clang++-18)
set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS /usr/bin/clang-scan-deps-18)
set(CMAKE_CXX_STANDARD 23)

# See Help/dev/experimental.rst
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project(Test LANGUAGES CXX)
set(CMAKE_CXX_MODULE_STD 1)

add_subdirectory("module-test")