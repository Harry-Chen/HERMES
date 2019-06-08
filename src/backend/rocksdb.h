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
#undef _ROCKS_DB_

#undef leveldb
#undef LevelDB
#undef LDB
#undef PathComparator

#else
#include "leveldb.h"
#endif

#if 0
// make GitHub happy
class do_not_recognize_me_as_c_header {};
#endif

#endif  // HERMES_ROCKSDB_H
