#ifndef __HERMES_H__
#define __HERMES_H__

#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>
#include <sys/stat.h>
#include <unistd.h>

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
    const char *kvdev;
    const char *filedev;
    int show_help;
  };
  
  extern hermes::options opts;

  /**
   * HERMES private data
   * -------------------
   *
   * Constructed at init and will be passed as context
   * at all operations.
   */
  template<typename Backend>
  struct basic_context {
    Backend *backend;

    inline
    basic_context(const options &opts) : backend(new Backend(opts)) {};
  };

  struct metadata {
    // Ignored/omitted fields:
    // Ref: https://libfuse.github.io/doxygen/structfuse__operations.html#ac39a0b7125a0e5001eb5ff42e05faa5d
    //
    // st_dev, st_ino is ignored
    // for files/symlinks, st_nlink is always 1, as we do not supports hard links.
    // for directories, st_nlink SHOULD calculated from the number of subdirectories,
    //     but since btrfs already omits special values (.. and .) in hard link counts, so we are also not doing it.
    //     Hence, st_nlinks is always 1 for directories as well.
    //     Ref: https://unix.stackexchange.com/questions/101515/why-does-a-new-directory-have-a-hard-link-count-of-2-before-anything-is-added-to
    // st_rdev is always 0, as we do not have device files either
    // st_blksize is also ignored
    // st_blocks is always ceil(size / 512) for now, as we do not support holes yet

    // mode
    mode_t mode;

    // Owner
    uid_t uid;
    gid_t gid;

    // Size
    // Note that vedis does support the STRLEN command. But since directories is very large, and files are non-volatile, so we are not using that
    // If we really needs to modify a file, vedis also supports transaction
    //
    // For symbolic links, this field repersents the length of the pathname it points to
    // For directories, the meaning of this field is undefined (ref: fstat(2)). ext4 gives 4096.
    off_t size;

    // (Confusing) timestamps
    struct timespec atim;
    struct timespec mtim;
    struct timespec ctim;

    inline
    struct stat to_stat() const {
      struct stat result {
        .st_dev = 0,
        .st_ino = 0,
        .st_mode = mode,
        .st_uid = uid,
        .st_gid = gid,
        .st_rdev = 0,
        .st_size = size,
        .st_blksize = 0,
        .st_blocks = (size + 511) / 512,
#ifndef __APPLE__
        .st_atim = atim,
        .st_mtim = mtim,
        .st_ctim = ctim,
#else
        .st_atimespec = atim,
        .st_mtimespec = mtim,
        .st_ctimespec = ctim,
#endif
      };

      result.st_nlink = 0;

      return result;
    };

    inline
    bool is_file() const {
      return (mode & S_IFMT) == S_IFREG;
    }

    inline
    bool is_dir() const {
      return (mode & S_IFMT) == S_IFDIR;
    }

    inline
    bool is_symlink() const {
      return (mode & S_IFMT) == S_IFLNK;
    }
  };

  namespace impl {
    void* init(struct fuse_conn_info *conn);
    void destroy(void *ctx);
    int getattr(const char *path, struct stat *stbuf);
    int mkdir(const char *path, mode_t mode);
    int unlink(const char *path);
    int chmod(const char *path, mode_t mode);
    int chown(const char *path, uid_t uid, gid_t gid);
    int truncate(const char *path, off_t len);
    int open(const char *path, struct fuse_file_info *fi);
    int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    int write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    int create(const char *path, mode_t mode, struct fuse_file_info *fi);
    int utimens(const char *path, const struct timespec tv[2]);
    int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
  }
}

#endif // __HERMES_H__
