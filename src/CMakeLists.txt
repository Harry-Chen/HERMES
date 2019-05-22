cmake_minimum_required(VERSION 2.8)

set(EXE HERMES)

add_executable(${EXE}
    hermes.cpp
)

target_include_directories(${EXE} PRIVATE ${FUSE_INCLUDE_DIR})
target_link_libraries(${EXE} ${FUSE_LIBRARIES})
target_link_libraries(${EXE} ${CMAKE_THREAD_LIBS_INIT})
