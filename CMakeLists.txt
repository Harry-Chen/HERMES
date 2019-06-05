cmake_minimum_required(VERSION 2.8)
project(HERMES LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -Ofast -march=native -mtune=native -Wall -g -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64")

set(EXE HERMES)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

option(BACKEND "Key-Value backend" OFF)
set(BACKEND_VALUES "BerkeleyDB" "LevelDB" "RocksDB" "vedis")
set_property(CACHE BACKEND PROPERTY STRINGS ${BACKEND_VALUES})
list(FIND BACKEND_VALUES ${BACKEND} BACKEND_INDEX)

if(${BACKEND_INDEX} EQUAL -1)
    message(WARNING "Backend ${BACKEND} is empty or not supported, valid items are ${BACKEND_VALUES}")
    set(BACKEND "LevelDB")
endif()

add_definitions("-DHERMES_BACKEND=\"${BACKEND}\"")
string(TOUPPER "${BACKEND}" BACKEND)
add_definitions("-DHERMES_BACKEND_${BACKEND}")

message(STATUS "HERMES backend: ${BACKEND}")

find_package(FUSE REQUIRED)
find_package(Threads REQUIRED)

include_directories(${FUSE_INCLUDE})

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h lib/*.h)

list(FILTER HEADERS EXCLUDE REGEX "src/backend/.*")
list(FILTER SOURCES EXCLUDE REGEX "src/backend/.*")

set(HEADERS ${HEADERS} src/backend/common.h)

include_directories("${HERMES_SOURCE_DIR}/src")

if(${BACKEND} STREQUAL VEDIS)

elseif(${BACKEND} STREQUAL LEVELDB)
    find_package(LevelDB REQUIRED)
    include_directories(${LevelDB_INCLUDE})
    set(HEADERS ${HEADERS} src/backend/leveldb.h)
    set(HEADERS ${SOURCES} src/backend/leveldb.cpp)
elseif(${BACKEND} STREQUAL ROCKSDB)
    find_package(RocksDB REQUIRED)
    include_directories(${RocksDB_INCLUDE_DIR})
    set(HEADERS ${HEADERS} src/backend/rocksdb.h)
    set(HEADERS ${SOURCES} src/backend/rocksdb.cpp)
elseif(${BACKEND} STREQUAL BERKELEYDB)

endif()

add_executable(${EXE} ${SOURCES} ${HEADERS} src/utils.h)

target_link_libraries(${EXE} ${FUSE_LIBRARIES})
target_link_libraries(${EXE} ${CMAKE_THREAD_LIBS_INIT})

if(${BACKEND} STREQUAL VEDIS)

elseif(${BACKEND} STREQUAL LEVELDB)
    target_link_libraries(${EXE} ${LevelDB_LIBRARIES})
elseif(${BACKEND} STREQUAL ROCKSDB)
    target_link_libraries(${EXE} ${RocksDB_LIBRARIES})
elseif(${BACKEND} STREQUAL BERKELEYDB)

endif()

