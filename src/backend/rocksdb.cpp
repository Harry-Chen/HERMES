#define leveldb rocksdb
#define LevelDB RocksDB
#define LDB RocksDB
#define PathComparator RocksDBPathComparator

#define _ROCKS_DB_
#include "leveldb.cpp"
#undef _ROCKS_DB_

#undef leveldb
#undef LevelDB
#undef LDB
#undef PathComparator
