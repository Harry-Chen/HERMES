#ifndef HERMES_ROCKSDB_H
#define HERMES_ROCKSDB_H

#ifndef _ROCKS_DB_  // not included from rocksdb.cpp

#define leveldb rocksdb
#define LevelDB RocksDB
#define LDB RocksDB
#define PathComparator RocksDBPathComparator

#define _ROCKS_DB_
#undef __BACKEND_LEVELDB_h__
#include "leveldb.h"
#undef _ROCKS_DB

#undef leveldb
#undef LevelDB
#undef LDB
#undef PathComparator

#else
#include "leveldb.h"
#endif

#endif  // HERMES_ROCKSDB_H
