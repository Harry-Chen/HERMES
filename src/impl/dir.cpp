#include "hermes.h"
#include "impl/common.h"

#include <cstring>
#include <unordered_map>
#include <optional>
#include <shared_mutex>

using namespace std;

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

struct PairHash {
    public:
        template <typename T, typename U>
        std::size_t operator()(const std::pair<T, U> &x) const {
            return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
        }
};


static unordered_map<pair<string, off_t>, string, PairHash> last_paths;
static shared_mutex last_paths_mutex;

int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
            struct fuse_file_info *fi, enum fuse_readdir_flags) {
    //TODO: support offsets and plus mode
 
    (void)offset;  // Yes, we do not support offsets. (for now)

    char name[hermes::PATHNAME_LIMIT];

    string_view pathView(path);

    auto fctx = fuse_get_context();
    auto ctx = static_cast<hermes::impl::context *>(fctx->private_data);

    if(offset < 1)
        filler(buf, ".", NULL, 1, (enum fuse_fill_dir_flags) 0);
    if(offset < 2)
        filler(buf, "..", NULL, 2, (enum fuse_fill_dir_flags) 0);

    optional<string> last_path = {};

    if(offset > 2) {
        shared_lock lock(last_paths_mutex);
        // cout<<"[HERMES] DEBUG: Skipping... Offset: "<<offset<<endl;
        auto content = last_paths.extract({ path, offset });
        if(!content.empty()) {
            // cout<<"[HERMES] DEBUG: Found last_path: "<<content.mapped()<<endl;
            last_path = { std::move(content.mapped()) }; // content handle is now invalid
        }
    }

    off_t counter = offset < 2 ? 2 : offset;

    ctx->backend->iterate_directory(
        pathView,
        offset - 2,
        last_path,
        [&](const string_view &fp, const hermes::metadata &metadata) -> bool {
            const size_t copied = fp.substr(pathView == "/" ? pathView.size() : pathView.size() + 1,
                                            string::npos)
                                      .copy(name, string::npos);
            name[copied] = '\0';

            const auto stat = metadata.to_stat();
            auto result = filler(buf, name, &stat, counter + 1, FUSE_FILL_DIR_PLUS);
            if(result == 1) { // Buffer full
                unique_lock lock(last_paths_mutex);
                last_paths.emplace(make_pair(path, counter), fp);
                return false;
            }

            ++counter;
            return true;
        });

    return 0;
}
}  // namespace hermes::impl
