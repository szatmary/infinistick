#define FUSE_USE_VERSION 31
#include "cache.hpp"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// Global cache
auto g_cache = std::shared_ptr<Cache>(nullptr);

static struct options {
    const char* filename;
    const char* contents;
    int show_help;
} options;

#define OPTION(t, p)                      \
    {                                     \
        t, offsetof(struct options, p), 1 \
    }

static const struct fuse_opt option_spec[] = {
    OPTION("--name=%s", filename),
    OPTION("--contents=%s", contents),
    FUSE_OPT_END
};

static void* _init(struct fuse_conn_info* conn, struct fuse_config* cfg)
{
    (void)conn;
    cfg->kernel_cache = 1;
    return nullptr;
}

// List the virtual block devices
static int _readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags)
{
    std::cerr << "running" << std::endl;

    (void)offset, (void)fi, (void)flags;
    if (path != std::string("/")) {
        return -ENOENT;
    }

    filler(buf, ".", nullptr, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", nullptr, 0, (fuse_fill_dir_flags)0);
    auto devices = g_cache->ListBlockDevices();
    std::cerr << "_readdir" << std::endl;
    for (const auto& dev : devices) {
        filler(buf, std::to_string(dev->deviceId).c_str(), nullptr, 0, (fuse_fill_dir_flags)0);
    }

    return 0;
}

static int _getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
{
    (void)fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (path == std::string("/")) {
        stbuf->st_mode = S_IFDIR | 0555; // Read only
        stbuf->st_nlink = 2;
        return 0;
    }

    int deviceId = 0;
    if (1 != std::sscanf(path, "/%d", &deviceId)) {
        return -ENOENT;
    }

    // TODO check return value! return -ENOENT; on err
    auto dev = g_cache->GetBlockDevice(deviceId);
    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_nlink = 1;
    stbuf->st_size = dev->blockCount * static_cast<off_t>(dev->blockSize);
    return 0;
}

static int _open(const char* path, struct fuse_file_info* fi)
{
    int deviceId = 0;
    if (1 != std::sscanf(path, "/%d", &deviceId)) {
        return -ENOENT;
    }

    auto dev = g_cache->GetBlockDevice(deviceId);

    // if (strcmp(path + 1, options.filename) != 0)
    //     return -ENOENT;
    // if ((fi->flags & O_ACCMODE) != O_RDONLY)
    //     return -EACCES;
    return 0;
}

static int _read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
    int deviceId = 0;
    if (1 != std::sscanf(path, "/%d", &deviceId)) {
        return -ENOENT;
    }

    std::cerr << "_read "
              << " " << size << " " << offset << std::endl;
    auto dev = g_cache->GetBlockDevice(deviceId);
    if (!dev) {
        return -ENOENT;
    }
    return dev->Read(buf, size, offset);

    // size_t len;
    // (void)fi;
    // if (strcmp(path + 1, options.filename) != 0)
    //     return -ENOENT;
    // len = strlen(options.contents);
    // if (offset < len) {
    //     if (offset + size > len)
    //         size = len - offset;
    //     memcpy(buf, options.contents + offset, size);
    // } else
    //     size = 0;
    // return size;
}

static int _write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
    int deviceId = 0;
    if (1 != std::sscanf(path, "/%d", &deviceId)) {
        return -ENOENT;
    }

    std::cerr << "_write "
              << " " << size << " " << offset << std::endl;
    auto dev = g_cache->GetBlockDevice(deviceId);
    if (!dev) {
        return -ENOENT;
    }
    return dev->Write(buf, size, offset);
}

// https://libfuse.github.io/doxygen/include_2fuse_8h.html
// https://libfuse.github.io/doxygen/structfuse__operations.html
static const struct fuse_operations oper = {
    .getattr = _getattr,
    .open = _open,
    .read = _read,
    .write = _write,
    .readdir = _readdir,
    .init = _init,
};

int main(int argc, char* argv[])
{
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    /* Set defaults -- we have to use strdup so that
           fuse_opt_parse can free the defaults if other
           values are specified */
    options.filename = strdup("hello");
    options.contents = strdup("Hello World!\n");
    /* Parse options */

    if (fuse_opt_parse(&args, &options, option_spec, nullptr) == -1)
        return 1;
    /* When --help is specified, first print our own file-system
           specific help text, then signal fuse_main to show
           additional help (by adding `--help` to the options again)
           without usage: line (by setting argv[0] to the empty
           string) */

    // TODO use a sane default path, And accept a path form command line
    g_cache = Cache::New("/tmp/test.db");
    // if (!g_cache) {
    //     return EXIT_FAILURE;
    // }
    g_cache->NewBlockDevice("test", 2 * 1024 * 1024, 1024);
    std::cerr << "running" << std::endl;

    ret = fuse_main(args.argc, args.argv, &oper, nullptr);
    fuse_opt_free_args(&args);
    return ret;
}
