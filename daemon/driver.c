#include "Ambianceduino.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>

#define __USE_BSD    /* For S_IFDIR */
#include <sys/stat.h>
#include <sys/types.h>
#include "utils.h"

#define FUSE_USE_VERSION 26
#include <fuse.h>

#define min(a, b) ((a) < (b)) ? (a) : (b)

HAL arduino;

typedef struct halfs_file {
   char * name;
   int mode;
   int (* size_callback)(const char *);
   int (* trunc_callback)(const char *);
   int (* read_callback)(const char *, char *, size_t, off_t);
   int (* write_callback)(const char *, const char *, size_t, off_t);
} halfs_file;

int default_size_callback(const char *path){
    return 0;
}

int default_trunc_callback(const char *path){
    return 0;
}

int default_read_callback(const char *path, char *buf, size_t size, off_t offset){
    return 0;
}

int default_write_callback(const char *path, const char *buf, size_t size, off_t offset){
    return 0;
}

int version_size(const char *path){
    return 40;
}

int version_read(const char * file, char * buffer, size_t size, off_t offset)
{
    char res[41] = {'\0'};
    int l = 40;

    if (offset != 0)
        return 0;

    HAL_READ(&arduino, version, res);
    l = min(((int)size), l);
    memcpy(buffer, res, l);
    return l;
}

int open_write(const char * file, const char * buffer, size_t size, off_t offset)
{
    if (buffer[0] == '1')
        HAL_on(&arduino);
    else
        HAL_off(&arduino);
    return size;
}

halfs_file all_paths[] = {
    {
        .name = "/version",
        .mode = 0444,
        .read_callback = version_read,
        .size_callback = version_size
    },
    {
        .name = "/open",
        .mode = 0222,
        .write_callback = open_write
    }
};

const size_t N_PATHS = sizeof(all_paths)/sizeof(struct halfs_file);

static int halfs_trunc(const char *path, off_t size){
    return 0;
}

static int halfs_getattr(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));

    if (streq(path, "/")) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        return 0;
    }
    for (size_t i = 0; i < N_PATHS; i++){
        if(streq(path, all_paths[i].name)){
            stbuf->st_mode = S_IFREG | all_paths[i].mode;
            stbuf->st_nlink = 1;
            stbuf->st_size = all_paths[i].size_callback(path);
            stbuf->st_uid = getuid();
            stbuf->st_gid = getgid();
            return 0;
        }
    }
    return -ENOENT;
}

void * halfs_init(struct fuse_conn_info *conn){
    HAL_init(&arduino, "/dev/ttyACM0", 115200);
    HAL_start(&arduino);
    return NULL;
}

static int halfs_open(const char *path, struct fuse_file_info *fi)
{
    for (size_t i = 0; i < N_PATHS; i++){
        if(streq(path, all_paths[i].name)){
            return 0;
        }
    }
    return -ENOENT;
}

static int halfs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
    for (size_t i = 0; i < N_PATHS; i++){
        if(streq(path, all_paths[i].name)){
            return all_paths[i].read_callback(path, buf, size, offset);
        }
    }
    return -ENOENT;
}


static int halfs_write(const char *path, const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {

    for (size_t i = 0; i < N_PATHS; i++){
        if(streq(path, all_paths[i].name)){
            return all_paths[i].write_callback(path, buf, size, offset);
        }
    }
    return -ENOENT;
}

static int halfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi) {

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    for (size_t i=0; i<N_PATHS; i++){
        filler(buf, all_paths[i].name + 1, NULL, 0);
    }

    return 0;
}

static struct fuse_operations hal_ops = {
    .getattr    = halfs_getattr,
    .readdir    = halfs_readdir,
    .open       = halfs_open,
    .read       = halfs_read,
    .write      = halfs_write,
    .truncate   = halfs_trunc,
    .init       = halfs_init
};

int main(int argc, char *argv[])
{
    for (size_t i = 0; i < N_PATHS; i++){
        if (all_paths[i].size_callback == NULL)
            all_paths[i].size_callback = default_size_callback;
        if (all_paths[i].trunc_callback == NULL)
            all_paths[i].trunc_callback = default_trunc_callback;
        if (all_paths[i].read_callback == NULL)
            all_paths[i].read_callback = default_read_callback;
        if (all_paths[i].write_callback == NULL)
            all_paths[i].write_callback = default_write_callback;
    }
    return fuse_main(argc, argv, &hal_ops, NULL);
}
