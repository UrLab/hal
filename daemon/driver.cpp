#include "Ambianceduino.hpp"
#include <iostream>
#include <unistd.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

using namespace std;

Ambianceduino * arduino;

typedef struct hal_file {
   char * name;
   int mode;
   int (* size_callback)(const char *);
   int (* trunc_callback)(const char *);
   int (* read_callback)(const char *, char *, size_t, off_t);
   int (* write_callback)(const char *, const char *, size_t, off_t);
} hal_file;

int default_size_callback(const char *){
    return 0;
}

int default_trunc_callback(const char *){
    return 0;
}

int default_read_callback(const char *, char *, size_t, off_t){
    return 0;
}

int default_write_callback(const char *, const char *, size_t, off_t){
    return 0;
}

int version_size(const char *){
    return 40;
}

int version_read(const char * file, char * buffer, size_t size, off_t offset)
{
    char res[100] = {'\0'};
    int l = 40;

    if (offset != 0)
        return 0;

    strcpy(res, arduino->version().c_str());
    memcpy(buffer, res, l+1);
    return l;
}

int open_write(const char * file, const char * buffer, size_t size, off_t offset){
    if(buffer[0] == '1')
        arduino->on();
    else
        arduino->off();

    return size;
}

hal_file all_paths[] = {
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
const size_t paths_size =  sizeof(all_paths)/sizeof(struct hal_file);

static inline bool streq(const char *s1, const char *s2)
{
    while (*s1 && *s2 && *s1 == *s2){
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

static int hal_trunc(const char *path, off_t size){
    return 0;
}

static int hal_getattr(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));

    if (streq(path, "/")) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        return 0;
    }
    for(int i = 0; i < paths_size; i++){
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

void * hal_init(struct fuse_conn_info *conn){
    arduino = new Ambianceduino("/dev/tty.usbmodemfd111");
}

static int hal_open(const char *path, struct fuse_file_info *fi)
{
    for(int i = 0; i < paths_size; i++){
        if(streq(path, all_paths[i].name)){
            return 0;
        }
    }
    return -ENOENT;

    //TODO check flags
}

static int hal_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
    for(int i = 0; i < paths_size; i++){
        if(streq(path, all_paths[i].name)){
            return all_paths[i].read_callback(path, buf, size, offset);
        }
    }
    return -ENOENT;
}


static int hal_write(const char *path, const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {

    for(int i = 0; i < paths_size; i++){
        if(streq(path, all_paths[i].name)){
            return all_paths[i].write_callback(path, buf, size, offset);
        }
    }
    return -ENOENT;
}

static int hal_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi) {

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    for (size_t i=0; i<paths_size; i++){
        filler(buf, all_paths[i].name + 1, NULL, 0);
    }

    return 0;
}

static struct fuse_operations hal_ops = {
    .getattr    = hal_getattr,
    .readdir    = hal_readdir,
    .open       = hal_open,
    .read       = hal_read,
    .write      = hal_write,
    .truncate   = hal_trunc,
    .init       = hal_init
};




int main(int argc, char *argv[])
{
    for(int i = 0; i < paths_size; i++){
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