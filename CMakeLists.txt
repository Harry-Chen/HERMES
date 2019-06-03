cmake_minimum_required(VERSION 2.8)
project(HERMES LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native -Wall -g -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64")

set(EXE HERMES)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/") 

find_package(Threads REQUIRED)
find_package(FUSE REQUIRED)
find_package(LevelDB REQUIRED)

include_directories(${FUSE_INCLUDE})
include_directories(${LevelDB_INCLUDE})

file(GLOB_RECURSE SRC_LIST src/*.cpp)
file(GLOB_RECURSE HDR_LIST src/*.h lib/*.h)

include_directories("${HERMES_SOURCE_DIR}/src")

add_executable(${EXE} ${SRC_LIST} ${HDR_LIST})

target_link_libraries(${EXE} ${FUSE_LIBRARIES})
target_link_libraries(${EXE} ${LevelDB_LIBRARIES})
target_link_libraries(${EXE} ${CMAKE_THREAD_LIBS_INIT})
