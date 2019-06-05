#ifndef __IMPL_COMMON_H__
#define __IMPL_COMMON_H__

#include "backend/leveldb.h"

namespace hermes::impl {
    /**
     * The selected context
     */
    using context = hermes::basic_context<hermes::backend::LDB>;
}
#endif // __IMPL_COMMON_H__
