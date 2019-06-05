#ifndef __BACKEND_COMMON_h__
#define __BACKEND_COMMON_h__

namespace hermes::backend {
    enum class write_result {
        Ok,
        PermissionDenied,
        UnknownFailure,
    };
};
#endif // __BACKEND_COMMON_h__
