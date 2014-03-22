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
DirTree *halfs_root = NULL;

const char *STATE_NAMES[] = {"STANDBY", "POWERED", "SHOWTIME", "ALERT"};

typedef struct halfs_file {
   char * name;
   char * target;
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

    if (offset >= 40)
        return 0;

    HAL_READ(&arduino, version, res);
    l = min(((int)size), l);
    memcpy(buffer, res + offset, l);
    return l;
}

int state_size(const char *path)
{
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

int trigger_size(const char *path)
{
    return 1;
}

int trigger_read(const char *file, char *buffer, size_t size, off_t offset)
{
    const char *trig = strrchr(file, '/')+1;
    int val = 0;

    if (streq(trig, "door")){
        HAL_READ(&arduino, door, val);
    }
    else if (streq(trig, "bell")){
        HAL_READ(&arduino, bell, val);
    }
    else if (streq(trig, "radiator")){
        HAL_READ(&arduino, radiator, val);
    }
    else if (streq(trig, "passage")){
        HAL_READ(&arduino, passage, val);
    }
    else if (streq(trig, "on")){
        HAL_READ(&arduino, on, val);
    }

    buffer[0] = val ? '1' : '0';
    return 1;
}

int anim_curve_write(const char *file, const char *buffer, size_t size, off_t offset)
{
    DirTree *parentdir = DirTree_findParent(halfs_root, file);
    if (parentdir){
        unsigned char len = min(size, 0xff);
        if (streq(parentdir->name, "R")){
            HAL_uploadAnim(&arduino, 0, len, (unsigned char*) buffer+offset);
            return len;
        }
        else if (streq(parentdir->name, "B")){
            HAL_uploadAnim(&arduino, 1, len, (unsigned char*) buffer+offset);
            return len;
        }
    }
    return -ENOENT;
}

int anim_fps_write(const char *file, const char *buffer, size_t size, off_t offset)
{
    DirTree *parentdir = DirTree_findParent(halfs_root, file);
    if (parentdir){
        char *endptr;
        int fps = strtol(buffer, &endptr, 10);
        if (endptr > buffer && fps > 0){
            if (fps > 0xff)
                fps = 0xff;
            if (streq(parentdir->name, "R")){
                HAL_setFPSAnim(&arduino, 0, fps);
                return size;
            }
            else if (streq(parentdir->name, "B")){
                HAL_setFPSAnim(&arduino, 1, fps);
                return size;
            }
        }
    }
    return -ENOENT;
}

int anim_reset_write(const char *file, const char *buffer, size_t size, off_t offset)
{
    DirTree *parentdir = DirTree_findParent(halfs_root, file);
    if (parentdir){
        if (streq(parentdir->name, "R")){
            HAL_resetAnim(&arduino, 0);
            return size;
        } else if (streq(parentdir->name, "B")){
            HAL_resetAnim(&arduino, 1);
            return size;
        }
    }
    return -ENOENT;
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
    /* Arduino state */
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
    /* Symlink test */
    {
        .name = "/demo-symlink",
        .mode = 0444,
        .target = "../driver.c"
    },

    /* Analog sensors: [0, 1023] */
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

    /* Digital sensors: {0, 1} */
    {
        .name = "/triggers/door",
        .mode = 0444,
        .read_callback = trigger_read,
        .size_callback = trigger_size
    },
    {
        .name = "/triggers/bell",
        .mode = 0444,
        .read_callback = trigger_read,
        .size_callback = trigger_size
    },
    {
        .name = "/triggers/radiator",
        .mode = 0444,
        .read_callback = trigger_read,
        .size_callback = trigger_size
    },
    {
        .name = "/triggers/passage",
        .mode = 0444,
        .read_callback = trigger_read,
        .size_callback = trigger_size
    },
    {
        .name = "/triggers/on",
        .mode = 0444,
        .read_callback = trigger_read,
        .size_callback = trigger_size
    },

    /* Ledstrips */
    {.name="/leds/R/curve", .mode=0222, .write_callback=anim_curve_write},
    {.name="/leds/R/fps"  , .mode=0222, .write_callback=anim_fps_write},
    {.name="/leds/R/reset", .mode=0222, .write_callback=anim_reset_write},
    {.name="/leds/B/curve", .mode=0222, .write_callback=anim_curve_write},
    {.name="/leds/B/fps"  , .mode=0222, .write_callback=anim_fps_write},
    {.name="/leds/B/reset", .mode=0222, .write_callback=anim_reset_write},

    /* open/close hackerspace */
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

    DirTree *file = DirTree_find(halfs_root, path);
    if (! file)
        return -ENOENT;

    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();

    if (file->first_child){
        /* has child => Directory */
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        halfs_file *fileopts = (halfs_file *) file->payload;
        if (fileopts->target == NULL){
            stbuf->st_mode = S_IFREG | fileopts->mode;
            stbuf->st_nlink = 1;
            stbuf->st_size = fileopts->size_callback(path);
        }
        else{
            stbuf->st_mode = S_IFLNK | fileopts->mode;
            stbuf->st_nlink = 1;
            stbuf->st_size = strlen(fileopts->target);
        }
    }

    return 0;
}

void * halfs_init(struct fuse_conn_info *conn)
{
    HAL_init(&arduino, "/dev/tty.usbmodemfa131", 115200);
    HAL_start(&arduino);
    HAL_askVersion(&arduino);

    halfs_root = DirTree_create("ROOT"); /* This name won't be used */
    for (size_t i=0; i<N_PATHS; i++){
        DirTree *file = DirTree_insert(halfs_root, all_paths[i].name);
        file->payload = (void*) &(all_paths[i]);
    }

    return NULL;
}

static int halfs_open(const char *path, struct fuse_file_info *fi)
{
    DirTree *file = DirTree_find(halfs_root, path);
    if (file)
        return 0;
    return -ENOENT;
}

static int halfs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
    DirTree *file = DirTree_find(halfs_root, path);
    if (file){
        halfs_file *fileopts = file->payload;
        return fileopts->read_callback(path, buf, size, offset);
    }
    return -ENOENT;
}


static int halfs_write(const char *path, const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi) {

    DirTree *file = DirTree_find(halfs_root, path);
    if (file){
        halfs_file *fileopts = file->payload;
        return fileopts->write_callback(path, buf, size, offset);
    }
    return -ENOENT;
}

static int halfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi) {

    DirTree *dir = DirTree_find(halfs_root, path);
    if (! dir)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (DirTree *it=dir->first_child; it!=NULL; it=it->next_sibling)
        filler(buf, it->name, NULL, 0);

    return 0;
}

static int halfs_readlink(const char *path, char *buf, size_t size){
    DirTree *file = DirTree_find(halfs_root, path);
    if (file){
        halfs_file *fileopts = file->payload;
        if(fileopts->target == NULL){
            return -ENOENT;
        }
        strcpy(buf, fileopts->target);
        return 0;
    }
}

static struct fuse_operations hal_ops = {
    .getattr    = halfs_getattr,
    .readdir    = halfs_readdir,
    .open       = halfs_open,
    .read       = halfs_read,
    .write      = halfs_write,
    .truncate   = halfs_trunc,
    .init       = halfs_init,
    .readlink   = halfs_readlink
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
