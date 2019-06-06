#include "impl/common.h"
#include "hermes.h"

namespace hermes::impl {
void *init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    (void) cfg;

    auto ctx = new hermes::impl::context(hermes::opts);
    return ctx;
}

void destroy(void *_ctx) {
    auto ctx = static_cast<hermes::impl::context *>(_ctx);
    delete ctx;
}
}  // namespace hermes::impl
