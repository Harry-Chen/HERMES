#include <unistd.h>
#include "hermes.h"
#include "impl/common.h"

#include <cstring>

#include <chrono>
#include <unordered_map>
#include <shared_mutex>

using namespace std;

namespace hermes::impl {
static unordered_map<uint64_t, uint64_t> pending_size;
static shared_mutex size_mutex;

int getattr(const char *path, struct stat *stbuf) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    // Is root
    if (strcmp(path, "/") == 0) {
        memset(stbuf, 0, sizeof(struct stat));
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 1;
        stbuf->st_uid = getuid();
        return 0;
    }

    auto resp = ctx->backend->fetch_metadata(path);

    if (resp) {
        *stbuf = resp->to_stat();
        return 0;
    } else {
        return -ENOENT;
    }
}

int chmod(const char *path, mode_t mode) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);
    auto resp = ctx->backend->fetch_metadata(path);

    if (!resp) {
        return -ENOENT;
    }

    resp->mode = (resp->mode & S_IFMT) + (mode & ~S_IFMT);

    ctx->backend->put_metadata(path, *resp);

    return 0;
}

int chown(const char *path, uid_t uid, gid_t gid) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);
    auto resp = ctx->backend->fetch_metadata(path);

    if (!resp) {
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

    if (!resp) {
        return -ENOENT;
    }

    resp->size = size;

    unique_lock lock(size_mutex);

    pending_size[resp->id] = size;
    ctx->backend->put_metadata(path, *resp);

    return 0;
}

int open(const char *path, struct fuse_file_info *fi) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    auto resp = ctx->backend->fetch_metadata(path);

    if (!resp) {
        return -ENOENT;
    } else if (!resp->is_file()) {
        return -EISDIR;
    } else {
        fi->fh = resp->id;
        unique_lock lock(size_mutex);
        if (pending_size.find(resp->id) == pending_size.end()) pending_size[resp->id] = resp->size;
        // cout<<">> FileID: "<<resp->id<<endl;
        return 0;
    }
}

int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    // cout<<">> Reading: "<<fi->fh<<endl;

    /*
    auto mtresp = ctx->backend->fetch_metadata(path);

    // TODO: upate atim
    // TODO: handle dir & symlink

    if(!mtresp) {
      return -ENOENT;
    } else if(mtresp->size == 0) {
      return 0;
    } else {
      ctx->backend->fetch_content(mtresp->id, offset, size, buf);
      return size;
    }
    */

    shared_lock lock(size_mutex);
    if (offset + size > pending_size[fi->fh]) {
        if (offset > pending_size[fi->fh]) return 0;

        size = pending_size[fi->fh] - offset;
    }

    // File size in store will never be shorten, so we can safely unlock here
    lock.unlock();

    ctx->backend->fetch_content(fi->fh, offset, size, buf);
    return size;
}

int write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    // TODO: Alter file size and mtime in release

    // auto mtresp = ctx->backend->fetch_metadata(path);

    // TODO: lock
    ctx->backend->put_content(fi->fh, offset, string_view(buf, size));

    /*
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    mtresp->mtim = now;
    mtresp->atim = now;
    if(offset + size > mtresp->size) mtresp->size = offset + size;
    ctx->backend->put_metadata(path, *mtresp);
    */

    uint64_t new_size = offset + size;

    unique_lock lock(size_mutex);
    uint64_t &saved_size = pending_size[fi->fh];  // uint64_t will be default-initialized into 0
    if (saved_size < new_size) saved_size = new_size;

    return size;
}

int release(const char *path, struct fuse_file_info *fi) {

    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    auto mtresp = ctx->backend->fetch_metadata(path);

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    // Written
    mtresp->mtim = now;

    shared_lock lock(size_mutex);
    // It's highly unlikely that there is a race in release,
    // (e.g., we open a file, and when releasing it, immediately open the file again and close it,
    // then we need the second release call to go faster than the first one to trigger a race)
    // so we delay the check for saved_size's presence
    auto saved_size = pending_size.find(fi->fh);
    if (saved_size == pending_size.end()) return 0;

    mtresp->size = saved_size->second;
    // We are not erasing the saved_size from cache, since we will have to reload it again in future.
    // Then we are able to use shared_lock here
    // pending_size.erase(saved_size);

    ctx->backend->put_metadata(path, *mtresp);
    // cout<<"Write"<<endl;
    return 0;
}

int create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    // auto before = std::chrono::high_resolution_clock::now();

    auto fctx = fuse_get_context();
    auto ctx = static_cast<hermes::impl::context *>(fctx->private_data);
    if (ctx->backend->fetch_metadata(path)) return -EEXIST;

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    hermes::metadata mt = {
        .id = ctx->backend->next_id(),
        .mode = mode,
        .uid = fctx->uid,
        .gid = fctx->gid,
        .size = 0,
        .atim = now,
        .mtim = now,
        .ctim = now,
    };

    // TODO: check permission and deal with errors
    ctx->backend->put_metadata(path, mt);
    fi->fh = mt.id;

    unique_lock lock(size_mutex);
    pending_size[mt.id] = 0;
    lock.unlock();

    // auto after = std::chrono::high_resolution_clock::now();
    // auto diff = after - before;

    // cout<<"Time used:
    // "<<std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count()<<endl;
    return 0;
}

int utimens(const char *path, const struct timespec tv[2]) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);
    auto resp = ctx->backend->fetch_metadata(path);

    if (!resp) {
        return -ENOENT;
    }

    resp->atim = tv[0];
    resp->mtim = tv[1];

    ctx->backend->put_metadata(path, *resp);

    return 0;
}

int unlink(const char *path) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);
    auto resp = ctx->backend->remove_metadata(path);
    if (resp) {
        return 0;
    }
    return -ENOENT;
}

int rename(const char *from, const char *to) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    /*
    if(flags == RENAME_EXCHANGE) {
      auto oriFrom = ctx->backend->remove_metadata(from);
      if(!oriFrom) return -ENOENT;

      auto oriTo = ctx->backend->remove_metadata(to);
      if(!oriTo) {
        ctx->backend->put_metadata(from, *oriFrom);
        return -ENOENT;
      }

      struct timespec now;
      clock_gettime(CLOCK_REALTIME, &now);

      oriFrom->mtim = now;
      oriTo->mtim = now;

      ctx->backend->put_metadata(to, *oriFrom);
      ctx->backend->put_metadata(from, *oriTo);

      const auto contFrom = ctx->backend->remove_content(from);
      const auto contTo = ctx->backend->remove_content(to);

      if(contFrom) ctx->backend->put_content(to, *contFrom);
      if(contTo) ctx->backend->put_content(from, *contTo);

      return 0;
    } else if(flags == RENAME_NOREPLACE) {
      if(ctx->backend->fetch_metadata(to))
        return -EEXIST;
    }
    */

    auto original = ctx->backend->remove_metadata(from);
    if (!original) return -ENOENT;

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    original->mtim = now;
    ctx->backend->put_metadata(to, *original);

    return 0;
}

int readlink(const char *path, char *buf, size_t size) {
    auto ctx = static_cast<hermes::impl::context *>(fuse_get_context()->private_data);

    auto resp = ctx->backend->fetch_metadata(path);

    if (!resp) {
        return -ENOENT;
    } else if (!resp->is_symlink()) {
        return -EINVAL;
    }

    size_t read_size = size - 1;
    if(read_size > resp->size) read_size = resp->size;
    ctx->backend->fetch_content(resp->id, 0, read_size, buf);
    buf[read_size] = 0;
    return 0;
}

int symlink(const char *path, const char *to) {
    auto fctx = fuse_get_context();
    auto ctx = static_cast<hermes::impl::context *>(fctx->private_data);
    if (ctx->backend->fetch_metadata(to)) return -EEXIST;

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    string_view path_view(path);

    hermes::metadata mt = {
        .id = ctx->backend->next_id(),
        .mode = (mode_t)(0 | S_IFLNK),
        .uid = fctx->uid,
        .gid = fctx->gid,
        .size = path_view.size(),
        .atim = now,
        .mtim = now,
        .ctim = now,
    };

    // TODO: check permission and deal with errors
    ctx->backend->put_metadata(to, mt);
    ctx->backend->put_content(mt.id, 0, path_view);

    return 0;
}

}  // namespace hermes::impl
