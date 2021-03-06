#ifndef __IMPL_COMMON_H__
#define __IMPL_COMMON_H__

#if defined(HERMES_BACKEND_VEDIS)
#include "backend/vedis.h"
#elif defined(HERMES_BACKEND_BERKELEYDB)
#include "backend/berkeleydb.h"
#elif defined(HERMES_BACKEND_LEVELDB)
#include "backend/leveldb.h"
#elif defined(HERMES_BACKEND_ROCKSDB)
#include "backend/rocksdb.h"
#endif

namespace hermes::impl {
/**
 * The selected context
 */
#if defined(HERMES_BACKEND_VEDIS)
using context = hermes::basic_context<hermes::backend::Vedis>;
#elif defined(HERMES_BACKEND_BERKELEYDB)
using context = hermes::basic_context<hermes::backend::BDB>;
#elif defined(HERMES_BACKEND_LEVELDB)
using context = hermes::basic_context<hermes::backend::LDB>;
#elif defined(HERMES_BACKEND_ROCKSDB)
using context = hermes::basic_context<hermes::backend::RocksDB>;
#endif

}  // namespace hermes::impl
#endif  // __IMPL_COMMON_H__
