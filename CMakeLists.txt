cmake_minimum_required(VERSION 2.8)
project(HERMES LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native -mtune=native -Wall -g -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64")

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "${CMAKE_SOURCE_DIR}/CMakeModules/")

find_package(Threads REQUIRED)
find_package(FUSE REQUIRED)

include_directories(".")

add_subdirectory(src)
