#include "hermes.h"
#include "impl/common.h"

#include <cstring>
#include <unistd.h>
#include <errno.h>

#include <iostream>

namespace hermes::impl {
  int getattr(const char *path, struct stat *stbuf) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    // Is root
    if(std::strcmp(path, "/") == 0) {
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink = 1;
      return 0;
    }

    auto resp = ctx->backend->fetch_metadata(path);

    if(resp) {
      *stbuf = resp->to_stat();
      return 0;
    } else {
      return -ENOENT;
    }
  }

  int chmod(const char *path, mode_t mode) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);
    auto resp = ctx->backend->fetch_metadata(path);

    if(!resp) {
      return -ENOENT;
    }

    resp->mode = (resp->mode & S_IFMT) + (mode & ~S_IFMT);

    ctx->backend->put_metadata(path, *resp);

    return 0;
  }

  int chown(const char *path, uid_t uid, gid_t gid) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);
    auto resp = ctx->backend->fetch_metadata(path);

    if(!resp) {
      return -ENOENT;
    }

    resp->uid = uid;
    resp->gid = gid;

    ctx->backend->put_metadata(path, *resp);

    return 0;
  }

  int truncate(const char *path, off_t size) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);
    auto resp = ctx->backend->fetch_metadata(path);

    if(!resp) {
      return -ENOENT;
    }

    resp->size = size;

    ctx->backend->put_metadata(path, *resp);

    return 0;
  }

  int open(const char *path, struct fuse_file_info *fi) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    auto resp = ctx->backend->fetch_metadata(path);

    // TODO: do we need to follow symbolic links here?
    if(!resp) {
      return -ENOENT;
    } else if(!resp->is_file()) {
      return -EISDIR;
    } else {
      std::cout<<">>>>> SIZE IS: "<<resp->size<<std::endl;
      return 0;
    }
  }

  int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    auto resp = ctx->backend->fetch_content(path);

    // TODO: upate atim

    if(!resp) {
      return -ENOENT;
    } else {
      const std::string_view view = *resp;
      size_t cnt = resp->size() - offset;
      if(cnt > size) cnt = size;

      view.substr(offset, cnt).copy(buf, std::string::npos);
      return cnt;
    }
  }

  int write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    // TODO: blocks
    auto resp = ctx->backend->fetch_content(path);
    
    // TODO: I forgot, do we actually alter the size here?
    auto mtresp = ctx->backend->fetch_metadata(path);

    if(!resp || !mtresp) {
      return -ENOENT;
    } else {
      // TODO: lock
      resp->resize(offset);
      resp->append(buf, size);
      // TODO: Ignores errors for now
      ctx->backend->put_content(path, *resp);

      struct timespec now;
      clock_getres(CLOCK_REALTIME, &now);

      mtresp->mtim = now;
      mtresp->atim = now;
      mtresp->size = resp->size();
      ctx->backend->put_metadata(path, *mtresp);
      return size;
    }
  }

  int create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    auto fctx = fuse_get_context();
    auto ctx = static_cast<hermes::impl::context *>(fctx->private_data);

    struct timespec now;
    clock_getres(CLOCK_REALTIME, &now);

    hermes::metadata mt = {
      .mode = mode,
      .uid = fctx->uid,
      .gid = fctx->gid,
      .size = 0,
      .atim = now,
      .mtim = now,
      .ctim = now,
    };

    // TODO: lock
    // TODO: check permission and deal with errors
    ctx->backend->put_metadata(path, mt);
    ctx->backend->put_content(path, "");
    return 0;
  }

  int utimens(const char *path, const struct timespec tv[2]) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);
    auto resp = ctx->backend->fetch_metadata(path);

    if(!resp) {
      return -ENOENT;
    }

    resp->atim = tv[0];
    resp->mtim = tv[1];

    ctx->backend->put_metadata(path, *resp);

    return 0;
  }
}
