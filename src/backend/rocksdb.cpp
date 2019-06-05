#define leveldb rocksdb
#define LevelDB RocksDB
#define LDB RocksDB
#define PathComparator RocksDBPathComparator

#define _ROCKS_DB_
#define HERMES_BACKEND_LEVELDB
#include "leveldb.cpp"
#undef HERMES_BACKEND_LEVELDB
#undef _ROCKS_DB_

#undef leveldb
#undef LevelDB
#undef LDB
#undef PathComparator
