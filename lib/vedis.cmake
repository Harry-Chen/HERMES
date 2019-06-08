set(VEDIS_DIR ${CMAKE_SOURCE_DIR}/lib)

if(NOT EXISTS "${VEDIS_DIR}/vedis/vedis.h")
    message(FATAL_ERROR "Vedis source code not present. Please update your git submodules!")
endif()

set(VEDIS_COMPILER_FLAGS "-DVEDIS_ENABLE_THREADS")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VEDIS_COMPILER_FLAGS}")

set(VEDIS_INCLUDE_DIR ${VEDIS_DIR})

add_library(vedis STATIC ${VEDIS_DIR}/vedis/vedis.c ${VEDIS_DIR}/vedis/vedis.h)

set(VEDIS_LIBRARY vedis)

file(STRINGS ${VEDIS_DIR}/vedis/vedis.h _VEDIS_VERSION_LINE REGEX "#define[ \t]+VEDIS_VERSION[ \t]+\"(.*)\".*")

if(${_VEDIS_VERSION_LINE} MATCHES "[^k]+VEDIS_VERSION[ \t]+\"(([0-9]\\.)+[0-9])\"")
    set(VEDIS_VERSION ${CMAKE_MATCH_1})
else()
    set(VEDIS_VERSION "Unknown")
endif()
