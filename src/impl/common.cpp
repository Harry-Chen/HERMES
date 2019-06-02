#include "hermes.h"
#include "impl/common.h"

#include <cassert>
#include <unistd.h>

namespace hermes::impl {
  void* init(struct fuse_conn_info *conn) {
    auto ctx = new hermes::impl::context(hermes::opts);
    return ctx;
  }

  void destroy(void *_ctx) {
    auto ctx = static_cast<hermes::impl::context *>(_ctx);
    delete ctx;
  }
}
