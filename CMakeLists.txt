cmake_minimum_required(VERSION 3.5)
project(large_file_copy)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c error_printer.c error_printer.h)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(large_file_copy ${SOURCE_FILES})