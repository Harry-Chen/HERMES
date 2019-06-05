# Find the RocksDB libraries
#
# The following variables are optionally searched for defaults
#  ROCKSDB_ROOT_DIR:    Base directory where all RocksDB components are found
#
# The following are set after configuration is done:
#  ROCKSDB_FOUND
#  RocksDB_INCLUDE_DIR
#  RocksDB_LIBRARIES

find_path(RocksDB_INCLUDE_DIR NAMES rocksdb/db.h
        PATHS ${ROCKSDB_ROOT_DIR} ${ROCKSDB_ROOT_DIR}/include)

find_library(RocksDB_LIBRARIES NAMES rocksdb
        PATHS ${ROCKSDB_ROOT_DIR} ${ROCKSDB_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RocksDB DEFAULT_MSG RocksDB_INCLUDE_DIR RocksDB_LIBRARIES)

if(ROCKSDB_FOUND)
    message(STATUS "Found RocksDB  (include: ${RocksDB_INCLUDE_DIR}, library: ${RocksDB_LIBRARIES})")
    mark_as_advanced(RocksDB_INCLUDE_DIR RocksDB_LIBRARIES)

    if(EXISTS "${RocksDB_INCLUDE_DIR}/rocksdb/version.h")
        file(STRINGS "${RocksDB_INCLUDE_DIR}/rocksdb/version.h" __version_lines
                REGEX "#define ROCKSDB_[^V]+[ \t]+[0-9]+")

        foreach(__line ${__version_lines})
            if(__line MATCHES "[^k]+ROCKSDB_MAJOR[ \t]+([0-9]+)")
                set(RocksDB_VERSION_MAJOR ${CMAKE_MATCH_1})
            elseif(__line MATCHES "[^k]+ROCKSDB_MINOR[ \t]+([0-9]+)")
                set(RocksDB_VERSION_MINOR ${CMAKE_MATCH_1})
            elseif(__line MATCHES "[^k]+ROCKSDB_PATCH[ \t]+([0-9]+)")
                set(RocksDB_VERSION_PATCH ${CMAKE_MATCH_1})
            endif()
        endforeach()

        if(RocksDB_VERSION_MAJOR AND RocksDB_VERSION_MINOR AND RocksDB_VERSION_PATCH)
            set(RocksDB_VERSION "${RocksDB_VERSION_MAJOR}.${RocksDB_VERSION_MINOR}.${RocksDB_VERSION_PATCH}")
        endif()
    endif()

endif()
