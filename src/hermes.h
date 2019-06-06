#ifndef __HERMES_H__
#define __HERMES_H__

#define FUSE_USE_VERSION 30

#include <fuse3/fuse.h>
#include <sys/stat.h>
#include <unistd.h>
#include <functional>
#include <string_view>

#include "utils.h"

namespace hermes {
// Maximum length for pathnames
const size_t PATHNAME_LIMIT = 4096;

/**
 * HERMES command-line options
 * ---------------------------
 *
 * Will be parsed by fuse_opt_parse.
 * Use 2 block devices to avoid problems in process
 * of opening the k-v database.
 */
struct options {
    const char *metadev;
    const char *filedev;
    int show_help;
    int show_version;
};

extern hermes::options opts;

/**
 * HERMES private data
 * -------------------
 *
 * Constructed at init and will be passed as context
 * at all operations.
 */

struct metadata {
    // Ignored/omitted fields:
    // Ref:
    // https://libfuse.github.io/doxygen/structfuse__operations.html#ac39a0b7125a0e5001eb5ff42e05faa5d
    //
    // st_dev, st_ino is ignored
    // for files/symlinks, st_nlink is always 1, as we do not supports hard links.
    // for directories, st_nlink SHOULD calculated from the number of subdirectories,
    //     but since btrfs already omits special values (.. and .) in hard link counts, so we are
    //     also not doing it. Hence, st_nlinks is always 1 for directories as well. Ref:
    //     https://unix.stackexchange.com/questions/101515/why-does-a-new-directory-have-a-hard-link-count-of-2-before-anything-is-added-to
    // st_rdev is always 0, as we do not have device files either
    // st_blksize is also ignored
    // st_blocks is always ceil(size / 512) for now, as we do not support holes yet

    // Internal file id
    // Only used for file I/O. Always 0 for directories
    uint64_t id;

    // mode
    mode_t mode;

    // Owner
    uid_t uid;
    gid_t gid;

    // Size
    // Note that vedis does support the STRLEN command. But since directories is very large, and
    // files are non-volatile, so we are not using that If we really needs to modify a file, vedis
    // also supports transaction
    //
    // For symbolic links, this field repersents the length of the pathname it points to
    // For directories, the meaning of this field is undefined (ref: fstat(2)). ext4 gives 4096.
    off_t size;

    // (Confusing) timestamps
    struct timespec atim;
    struct timespec mtim;
    struct timespec ctim;

    inline struct stat to_stat() const {
        struct stat result {
            .st_dev = 0, .st_ino = 0, .st_mode = mode, .st_uid = uid, .st_gid = gid, .st_rdev = 0,
            .st_size = size, .st_blksize = 0, .st_blocks = (size + 511) / 512,
#ifndef __APPLE__
            .st_atim = atim, .st_mtim = mtim, .st_ctim = ctim,
#else
            .st_atimespec = atim, .st_mtimespec = mtim, .st_ctimespec = ctim,
#endif
        };

        result.st_nlink = 0;

        return result;
    };

    inline bool is_file() const { return (mode & S_IFMT) == S_IFREG; }

    inline bool is_dir() const { return (mode & S_IFMT) == S_IFDIR; }

    inline bool is_symlink() const { return (mode & S_IFMT) == S_IFLNK; }
};

namespace backend {
    template <typename R>
    using DirectoryIterator = std::function<void(const R &, const metadata &)>;
}

HAS_MEMBER_FUNC_CHECKER(fetch_metadata);

HAS_MEMBER_FUNC_CHECKER(put_metadata);

HAS_MEMBER_FUNC_CHECKER(remove_metadata);

HAS_MEMBER_FUNC_CHECKER(put_content);

HAS_MEMBER_FUNC_CHECKER(fetch_content);

HAS_MEMBER_FUNC_CHECKER(remove_content);

HAS_MEMBER_FUNC_CHECKER(next_id);

HAS_MULTIPLE_GENERIC_MEMBER_FUNC_CHECK(iterate_directory);

template <typename Backend>
struct basic_context {
    Backend *backend;

    ASSERT_HAS_MEMBER_FUNC(Backend, fetch_metadata);
    ASSERT_HAS_MEMBER_FUNC(Backend, put_metadata);
    ASSERT_HAS_MEMBER_FUNC(Backend, remove_metadata);
    ASSERT_HAS_MEMBER_FUNC(Backend, put_content);
    ASSERT_HAS_MEMBER_FUNC(Backend, fetch_content);
    ASSERT_HAS_MEMBER_FUNC(Backend, remove_content);
    ASSERT_HAS_MEMBER_FUNC(Backend, next_id);
    /*
    // FIXME: clang++ does not report errors even if all specified overloads do not compile
    static_assert(HAS_MULTIPLE_GENERIC_MEMBER_FUNC_CHECKER_NAME(iterate_directory) < Backend,
                  backend::DirectoryIterator<std::string_view>,
                  backend::DirectoryIterator<std::string>,
                  backend::DirectoryIterator<char *>> ::value);
    */

    inline basic_context(const options &opts) : backend(new Backend(opts)){};
    ~basic_context() { delete backend; }
};

namespace impl {
    void *init(struct fuse_conn_info *conn, struct fuse_config *cfg);

    void destroy(void *ctx);

    int getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);

    int mkdir(const char *path, mode_t mode);

    int unlink(const char *path);

    int rename(const char *from, const char *to, unsigned int flags);

    int chmod(const char *path, mode_t mode, struct fuse_file_info *fi);

    int chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi);

    int truncate(const char *path, off_t len, struct fuse_file_info *fi);

    int open(const char *path, struct fuse_file_info *fi);

    int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

    int write(const char *path, const char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi);

    int release(const char *fh, struct fuse_file_info *fi);

    int create(const char *path, mode_t mode, struct fuse_file_info *fi);

    int utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi);

    int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                struct fuse_file_info *fi, enum fuse_readdir_flags);
}  // namespace impl
}  // namespace hermes

#endif  // __HERMES_H__
