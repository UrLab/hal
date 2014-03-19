#include "Ambianceduino.hpp"
#include <iostream>
#include <unistd.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

using namespace std;

Ambianceduino * arduino;

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
    } else if (streq(path, "/open")) {
        stbuf->st_mode = S_IFREG | 0222;
        stbuf->st_nlink = 1;
        stbuf->st_size = 50;
    } else if (streq(path, "/version")) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 50;
    } else {
        return -ENOENT;
    }

    return 0;
}

void * hal_init(struct fuse_conn_info *conn){
	arduino = new Ambianceduino("/dev/tty.usbmodemfd111");
}

static int hal_open(const char *path, struct fuse_file_info *fi)
{
    if (streq(path, "/open") || streq(path, "/version"))
    	return 0;

    return -ENOENT;

    //TODO check flags
}

static int hal_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
    char res[100] = {'\0'};
    int l = 40;

    if (offset != 0)
        return 0;

    if (streq(path, "/version"))
        strcpy(res, arduino->version().c_str());
    else if (streq(path, "/open"))
        return 0;
    else
        return -ENOENT;

    memcpy(buf, res, l+1);
    return l;
}


static int hal_write(const char *path, const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {

    if (streq(path, "/open")){
    	if(buf[0] == '1')
    		arduino->on();
    	else
    		arduino->off();
    }
    else
        return -EACCES;

    return size;
}

static struct fuse_operations hal_ops = {
    .getattr    = hal_getattr,
    // .readdir    = hal_readdir,
    .open       = hal_open,
    .read       = hal_read,
    .write      = hal_write,
    .truncate   = hal_trunc,
    .init	= hal_init
};




int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hal_ops, NULL);
}