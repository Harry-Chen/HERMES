#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "hermes.h"

fuse_operations hermes_oper = {
    .getattr = hermes::impl::getattr,
    .readlink = hermes::impl::readlink,
    .mkdir = hermes::impl::mkdir,
    .unlink = hermes::impl::unlink,
    .rmdir = hermes::impl::unlink,  // Shares impl with unlink for now
    .symlink = hermes::impl::symlink,
    .rename = hermes::impl::rename,
    .chmod = hermes::impl::chmod,
    .chown = hermes::impl::chown,
    .truncate = hermes::impl::truncate,
    .open = hermes::impl::open,
    .read = hermes::impl::read,
    .write = hermes::impl::write,
    .release = hermes::impl::release,
    .readdir = hermes::impl::readdir,
    .init = hermes::impl::init,
    .destroy = hermes::impl::destroy,
    .create = hermes::impl::create,
    .utimens = hermes::impl::utimens,
};

namespace hermes {
hermes::options opts;
}

#define DEFAULT_META_DEV "/tmp/fuse/kv"
#define DEFAULT_FILE_DEV "/tmp/fuse/file"
#define FUSE_OPTION(t, p, v) \
    { t, offsetof(hermes::options, p), v }

static struct fuse_opt option_spec[] = {FUSE_OPTION("--metadev=%s", metadev, 0),
                                        FUSE_OPTION("--filedev=%s", filedev, 0),
                                        FUSE_OPTION("-h", show_help, 1),
                                        FUSE_OPTION("--help", show_help, 1),
                                        FUSE_OPTION("-V", show_version, 1),
                                        FUSE_OPTION("--version", show_version, 1),
                                        FUSE_OPT_END};

static void show_help(const char *progname) {
    printf("usage: %s [options] <mountpoint>\n\n", progname);
    printf(
        "HERMES specific options:\n"
        "    --metadev=<s>       Path of file/device to store metadata\n"
        "                        (default: \"" DEFAULT_META_DEV
        "\")\n"
        "    --filedev=<s>       Path of file/device of main storage\n"
        "                        (default: \"" DEFAULT_FILE_DEV
        "\")\n"
        "\n");
}

static void show_version() {
    printf("HERMES: sHallow dirEctory stRucture Many-filE fileSystem\n");
    printf("Compilation Time: %s %s\n", __DATE__, __TIME__);
    printf("Key-Value Backend: %s %s\n", HERMES_BACKEND, HERMES_BACKEND_VERSION);
    puts("");
}

int main(int argc, char *argv[]) {
    int ret = 0;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    hermes::opts.metadev = strdup(DEFAULT_META_DEV);
    hermes::opts.filedev = strdup(DEFAULT_FILE_DEV);

    if (fuse_opt_parse(&args, &hermes::opts, option_spec, nullptr) == -1) return -1;

    if (hermes::opts.show_help) {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    } else if (hermes::opts.show_version) {
        show_version();
        assert(fuse_opt_add_arg(&args, "--version") == 0);
    }

    ret = fuse_main(args.argc, args.argv, &hermes_oper, nullptr);
    fuse_opt_free_args(&args);
    return ret;
}
