#include <stdio.h>
#include <stdlib.h>

#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>

fuse_operations hermes_oper;

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &hermes_oper, nullptr);
    return 0;
}
