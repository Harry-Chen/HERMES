#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "hermes.h"

fuse_operations hermes_oper = {
    .getattr = hermes::impl::getattr,
    .mkdir = hermes::impl::mkdir,
    .unlink = hermes::impl::unlink,
    .rmdir = hermes::impl::unlink,  // Shares impl with unlink for now
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
#define FUSE_OPTION(t, p) \
    { t, offsetof(hermes::options, p), 1 }

static struct fuse_opt option_spec[] = {
    FUSE_OPTION("--metadev=%s", metadev), FUSE_OPTION("--filedev=%s", filedev),
    FUSE_OPTION("-h", show_help), FUSE_OPTION("--help", show_help), FUSE_OPT_END};

static void show_help(const char *progname) {
    printf("HERMES: sHallow dirEctory stRucture Many-filE fileSystem\n\n");
    printf("usage: %s [options] <mountpoint>\n", progname);
    printf(
        "File-system specific options:\n"
        "    --metadev=<s>       Path of file/device to store metadata\n"
        "                        (default: \"" DEFAULT_META_DEV
        "\")\n"
        "    --filedev=<s>       Path of file/device of main storage\n"
        "                        (default: \"" DEFAULT_FILE_DEV
        "\")\n"
        "\n");
}

int main(int argc, char *argv[]) {
    int ret = 0;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    hermes::opts.metadev = strdup(DEFAULT_META_DEV);
    hermes::opts.filedev = strdup(DEFAULT_FILE_DEV);

    if (fuse_opt_parse(&args, &hermes::opts, option_spec, NULL) == -1) return -1;
    if (hermes::opts.show_help) {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    }

    ret = fuse_main(argc, argv, &hermes_oper, nullptr);
    fuse_opt_free_args(&args);
    return ret;
}
