#ifndef __BACKEND_COMMON_h__
#define __BACKEND_COMMON_h__

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>

#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#endif

namespace hermes::backend {
enum class write_result {
    Ok,
    PermissionDenied,
    UnknownFailure,
};
};
#endif  // __BACKEND_COMMON_h__
