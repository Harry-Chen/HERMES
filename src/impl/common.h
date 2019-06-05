#ifndef __IMPL_COMMON_H__
#define __IMPL_COMMON_H__

#include "backend/leveldb.h"
#include "backend/rocksdb.h"

namespace hermes::impl {
/**
 * The selected context
 */
using context = hermes::basic_context<hermes::backend::LDB>;
}  // namespace hermes::impl
#endif  // __IMPL_COMMON_H__
