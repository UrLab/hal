#include "Ambianceduino.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <stdio.h>

#define __USE_BSD    /* For S_IFDIR */
#include <sys/stat.h>
#include <sys/types.h>
#include "utils.h"
#include "dirtree.h"

#define FUSE_USE_VERSION 26
#include <fuse.h>

#define min(a, b) ((a) < (b)) ? (a) : (b)

HAL arduino;
Node *halfs_root = NULL;

const char *STATE_NAMES[] = {"STANDBY", "POWERED", "SHOWTIME", "ALERT"};

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

/* cat /version */
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

int state_size(const char *path){
    HALState state;
    HAL_READ(&arduino, state, state);
    if (state <= ALERT)
        return strlen(STATE_NAMES[state]);
    return 0;
}

/* cat /state */
int state_read(const char *file, char *buffer, size_t size, off_t offset)
{
    HALState state;
    HAL_READ(&arduino, state, state);

    if (state <= ALERT){
        int l = min(strlen(STATE_NAMES[state]), size);
        memcpy(buffer, STATE_NAMES[state], l);
        return l;
    }
    return 0;
}

int sensor_size(const char *path)
{
    return 4;
}


/* "temp_radia", "light_out", "temp_amb", "light_in", "temp_lm35", "Analog5" */

/* cat /sensor/<name> */
int sensor_read(const char *file, char *buffer, size_t size, off_t offset)
{
    const char *sensor = strrchr(file, '/')+1;
    int val = 0;

    if (streq(sensor, "light_inside")){
        HAL_askAnalog(&arduino, 3);
        HAL_READ(&arduino, light_inside, val);
    }
    else if (streq(sensor, "light_outside")){
        HAL_askAnalog(&arduino, 1);
        HAL_READ(&arduino, light_outside, val);
    }
    else if (streq(sensor, "temp_ambiant")){
        HAL_askAnalog(&arduino, 2);
        HAL_READ(&arduino, temp_ambiant, val);
    }
    else if (streq(sensor, "temp_radiator")){
        HAL_askAnalog(&arduino, 0);
        HAL_READ(&arduino, temp_radiator, val);
    }
    
    snprintf(buffer, size, "%4d", val);
    return min(size, 4);
}

/*! echo > /open */
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
        .name = "/state",
        .mode = 0444,
        .read_callback = state_read,
        .size_callback = state_size
    },
    {
        .name = "/sensors/light_inside",
        .mode = 0444,
        .read_callback = sensor_read,
        .size_callback = sensor_size
    },
    {
        .name = "/sensors/light_outside",
        .mode = 0444,
        .read_callback = sensor_read,
        .size_callback = sensor_size
    },
    {
        .name = "/sensors/temp_ambiant",
        .mode = 0444,
        .read_callback = sensor_read,
        .size_callback = sensor_size
    },
    {
        .name = "/sensors/temp_radiator",
        .mode = 0444,
        .read_callback = sensor_read,
        .size_callback = sensor_size
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

    Node *file = Node_find(halfs_root, path);
    if (! file)
        return -ENOENT;

    if (file->first_child){
        /* Directory */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
    } else {
        halfs_file *fileopts = (halfs_file *) file->payload;

        stbuf->st_mode = S_IFREG | fileopts->mode;
        stbuf->st_nlink = 1;
        stbuf->st_size = fileopts->size_callback(path);
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
    }

    return 0;
}

void * halfs_init(struct fuse_conn_info *conn)
{
    HAL_init(&arduino, "/dev/ttyACM0", 115200);
    HAL_start(&arduino);
    HAL_askVersion(&arduino);

    halfs_root = Node_create("ROOT"); /* This name won't be used */
    for (size_t i=0; i<N_PATHS; i++){
        Node *file = Node_insert(halfs_root, all_paths[i].name);
        file->payload = (void*) &(all_paths[i]);
    }

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

    Node *dir = Node_find(halfs_root, path);
    if (! dir)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (Node *it=dir->first_child; it!=NULL; it=it->next_sibling)
        filler(buf, it->name, NULL, 0);

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
