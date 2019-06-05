#include "hermes.h"
#include "impl/common.h"

#include <cstring>

namespace hermes::impl {
int mkdir(const char *path, mode_t mode) {
    auto fctx = fuse_get_context();
    auto ctx = static_cast<hermes::impl::context *>(fctx->private_data);
    if (ctx->backend->fetch_metadata(path)) return -EEXIST;

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    hermes::metadata mt = {
        .id = 0,

        .mode = (mode_t)(mode | S_IFDIR),

        .uid = fctx->uid,
        .gid = fctx->gid,

        .size = 4096,  // This will never change again

        .atim = now,
        .mtim = now,
        .ctim = now,
    };

    ctx->backend->put_metadata(path, mt);

    // No content for dir

    return 0;
}

int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
            struct fuse_file_info *fi) {
    (void)offset;  // Yes, we do not support offsets. (for now)

    char name[hermes::PATHNAME_LIMIT];

    std::string_view pathView(path);

    auto fctx = fuse_get_context();
    auto ctx = static_cast<hermes::impl::context *>(fctx->private_data);
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    ctx->backend->iterate_directory(
        pathView, [&](const std::string_view &fp, const hermes::metadata &metadata) -> void {
            const size_t copied = fp.substr(pathView == "/" ? pathView.size() : pathView.size() + 1,
                                            std::string::npos)
                                      .copy(name, std::string::npos);
            name[copied] = '\0';

            const auto stat = metadata.to_stat();
            filler(buf, name, &stat, 0);
        });

    return 0;
}
}  // namespace hermes::impl
