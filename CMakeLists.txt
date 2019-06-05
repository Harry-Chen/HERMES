cmake_minimum_required(VERSION 2.8)
project(HERMES LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -Ofast -march=native -mtune=native -Wall -g -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64")

set(EXE HERMES)


# find mandatory dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")
find_package(FUSE REQUIRED)
find_package(Threads REQUIRED)
include_directories(${FUSE_INCLUDE})


# find KV backend
option(BACKEND "Key-Value backend" OFF)
set(BACKEND_VALUES "BerkeleyDB" "LevelDB" "RocksDB" "vedis")
set_property(CACHE BACKEND PROPERTY STRINGS ${BACKEND_VALUES})
list(FIND BACKEND_VALUES ${BACKEND} BACKEND_INDEX)

if(${BACKEND_INDEX} EQUAL -1)
    message(WARNING "Backend ${BACKEND} not supported, valid items are: ${BACKEND_VALUES}")
    set(BACKEND "LevelDB")
endif()


# add macro definition for backend
add_definitions("-DHERMES_BACKEND=\"${BACKEND}\"")
message(STATUS "HERMES backend: ${BACKEND}")
string(TOUPPER "${BACKEND}" BACKEND)
add_definitions("-DHERMES_BACKEND_${BACKEND}")


# add backend-independent source files
file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h lib/*.h)
# remove backend-dependent files
list(FILTER HEADERS EXCLUDE REGEX "src/backend/.*")
list(FILTER SOURCES EXCLUDE REGEX "src/backend/.*")
set(HEADERS ${HEADERS} src/backend/common.h)
include_directories("${HERMES_SOURCE_DIR}/src")


# find backend library, add backend-specific files and detect version
if(${BACKEND} STREQUAL VEDIS)

elseif(${BACKEND} STREQUAL LEVELDB)
    find_package(LevelDB REQUIRED)
    include_directories(${LevelDB_INCLUDE})
    set(HEADERS ${HEADERS} src/backend/leveldb.h)
    set(SOURCES ${SOURCES} src/backend/leveldb.cpp)
    set(BACKEND_VERSION ${LEVELDB_VERSION})
elseif(${BACKEND} STREQUAL ROCKSDB)
    find_package(RocksDB REQUIRED)
    include_directories(${RocksDB_INCLUDE_DIR})
    set(HEADERS ${HEADERS} src/backend/rocksdb.h)
    set(SOURCES ${SOURCES} src/backend/rocksdb.cpp)
    set(BACKEND_VERSION ${RocksDB_VERSION})
elseif(${BACKEND} STREQUAL BERKELEYDB)
    find_package(BerkeleyDB REQUIRED)
    include_directories(${BerkeleyDB_INCLUDE_DIRS})
    set(HEADERS ${HEADERS} src/backend/berkeleydb.h)
    set(SOURCES ${SOURCES} src/backend/berkeleydb.cpp)
    set(BACKEND_VERSION ${BerkeleyDB_VERSION})
endif()

message(STATUS "HERMES backend version: ${BACKEND_VERSION}")
add_definitions("-DHERMES_BACKEND_VERSION=\"${BACKEND_VERSION}\"")


# compile the executable
add_executable(${EXE} ${SOURCES} ${HEADERS} src/utils.h)
target_link_libraries(${EXE} ${FUSE_LIBRARIES})
target_link_libraries(${EXE} ${CMAKE_THREAD_LIBS_INIT})


# link to backend library
if(${BACKEND} STREQUAL VEDIS)

elseif(${BACKEND} STREQUAL LEVELDB)
    target_link_libraries(${EXE} ${LevelDB_LIBRARIES})
elseif(${BACKEND} STREQUAL ROCKSDB)
    target_link_libraries(${EXE} ${RocksDB_LIBRARIES})
elseif(${BACKEND} STREQUAL BERKELEYDB)
    target_link_libraries(${EXE} ${BerkeleyDB_Cxx_LIBRARY})
endif()
