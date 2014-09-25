#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <stdio.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fuse.h>
#include "HALFS.h"
#include "com.h"
#include "utils.h"

static uid_t my_uid;
static gid_t my_gid;
static pthread_t com_thread;

/* Root of virtual file system */
HALFS *HALFS_root = NULL;

/* Main HAL structure */
static struct HAL_t hal;

/* Attempt to find arduino in these path */
const char *ARDUINO_DEV_PATH[] = {"/dev/tty.usbmodem*", "/dev/ttyUSB*", "/dev/ttyACM*"};

int version_size(HALResource *backend){return 41;}

/* cat /version */
int version_read(HALResource *backend, char * buffer, size_t size, off_t offset)
{
    if (offset > 40)
        return 0;
    int l = min(size, (size_t) 41-offset);
    sprintf(buffer, "%s\n", hal.version+offset);
    return l;
}

int trigger_read(HALResource *trig, char *buffer, size_t size, off_t offset)
{
    pthread_mutex_lock(&trig->mutex);
    HAL_ask_trigger(&hal, trig->id);
    pthread_cond_wait(&trig->cond, &trig->mutex);
    strcpy(buffer, (trig->data.b) ? "1\n" : "0\n");
    pthread_mutex_unlock(&trig->mutex);
    return 2;
}

int binary_size(HALResource *backend){return 2;}

int switch_read(HALResource *sw, char *buffer, size_t size, off_t offset)
{
    pthread_mutex_lock(&sw->mutex);
    HAL_ask_switch(&hal, sw->id);
    pthread_cond_wait(&sw->cond, &sw->mutex);
    strcpy(buffer, sw->data.b ? "1\n" : "0\n");
    pthread_mutex_unlock(&sw->mutex);
    return 2;
}

int switch_write(HALResource *sw, const char *buffer, size_t size, off_t offset)
{
    long int val = strtol(buffer, NULL, 10);
    pthread_mutex_lock(&sw->mutex);
    HAL_set_switch(&hal, sw->id, val ? true : false);
    pthread_mutex_unlock(&sw->mutex);
    return size;
}

int animation_upload(HALResource *anim, const char *buffer, size_t size, off_t offset)
{
    unsigned char s = size&0xff;
    pthread_mutex_lock(&anim->mutex);
    HAL_upload_anim(&hal, anim->id, s, (const unsigned char*) buffer);
    pthread_mutex_unlock(&anim->mutex);
    return s;
}

int anim_loop_write(HALResource *sw, const char *buffer, size_t size, off_t offset)
{
    long int val = strtol(buffer, NULL, 10);
    pthread_mutex_lock(&sw->mutex);
    HAL_set_anim_loop(&hal, sw->id, val ? true : false);
    pthread_mutex_unlock(&sw->mutex);
    return size;
}

int anim_loop_read(HALResource *anim, char *buffer, size_t size, off_t offset)
{
    pthread_mutex_lock(&anim->mutex);
    HAL_ask_anim_loop(&hal, anim->id);
    pthread_cond_wait(&anim->cond, &anim->mutex);
    strcpy(buffer, (anim->data.hhu4[0]) ? "1\n" : "0\n");
    pthread_mutex_unlock(&anim->mutex);
    return 2;
}

int anim_play_write(HALResource *sw, const char *buffer, size_t size, off_t offset)
{
    long int val = strtol(buffer, NULL, 10);
    pthread_mutex_lock(&sw->mutex);
    HAL_set_anim_play(&hal, sw->id, val ? true : false);
    pthread_mutex_unlock(&sw->mutex);
    return size;
}

int anim_play_read(HALResource *anim, char *buffer, size_t size, off_t offset)
{
    pthread_mutex_lock(&anim->mutex);
    HAL_ask_anim_play(&hal, anim->id);
    pthread_cond_wait(&anim->cond, &anim->mutex);
    strcpy(buffer, (anim->data.hhu4[1]) ? "1\n" : "0\n");
    pthread_mutex_unlock(&anim->mutex);
    return 2;
}

int anim_fps_write(HALResource *sw, const char *buffer, size_t size, off_t offset)
{
    long int fps = strtol(buffer, NULL, 10);
    if (fps < 4)         fps = 4;
    else if (fps > 1000) fps = 1000;
    unsigned int delay = 1000/fps;
    pthread_mutex_lock(&sw->mutex);
    HAL_set_anim_delay(&hal, sw->id, delay);
    pthread_mutex_unlock(&sw->mutex);
    return size;
}

int anim_fps_read(HALResource *anim, char *buffer, size_t size, off_t offset)
{
    int res = 0;
    pthread_mutex_lock(&anim->mutex);
    HAL_ask_anim_delay(&hal, anim->id);
    pthread_cond_wait(&anim->cond, &anim->mutex);
    unsigned int fps = 1000/anim->data.hhu4[2];
    snprintf(buffer, size, "%hhu\n%n", fps, &res);
    pthread_mutex_unlock(&anim->mutex);
    return res;
}

int anim_fps_size(HALResource *backend){return 4;}

int sensor_size(HALResource *backend){return 13;}

int sensor_read(HALResource *sensor, char *buffer, size_t size, off_t offset)
{
    int res = 0;
    pthread_mutex_lock(&sensor->mutex);
    HAL_ask_sensor(&hal, sensor->id);
    pthread_cond_wait(&sensor->cond, &sensor->mutex);
    snprintf(buffer, size, "%f\n%n", sensor->data.f, &res);
    pthread_mutex_unlock(&sensor->mutex);
    return res;
}

/*
 * Build HALFS tree structure from detected I/O
 */
static void HALFS_build()
{
    char path[128];
    HALFS_root = HALFS_create("/");

    my_uid = getuid();
    my_gid = getgid();

    HALFS * file = HALFS_insert(HALFS_root, "/version");
    file->ops.read = version_read;
    file->ops.size = version_size;

    file = HALFS_insert(HALFS_root, "/events");
    file->ops.mode = 0444;
    file->ops.target = "/tmp/hal.sock";
    
    for (size_t i=0; i<hal.n_triggers; i++){
        strcpy(path, "/triggers/");
        strcat(path, hal.triggers[i].name);
        file = HALFS_insert(HALFS_root, path);
        file->backend = hal.triggers + i;
        file->ops.read = trigger_read;
        file->ops.size = binary_size;
    }

    for (size_t i=0; i<hal.n_sensors; i++){
        strcpy(path, "/sensors/");
        strcat(path, hal.sensors[i].name);
        file = HALFS_insert(HALFS_root, path);
        file->backend = hal.sensors + i;
        file->ops.read = sensor_read;
        file->ops.size = sensor_size;
    }

    for (size_t i=0; i<hal.n_switchs; i++){
        strcpy(path, "/switchs/");
        strcat(path, hal.switchs[i].name);
        file = HALFS_insert(HALFS_root, path);
        file->backend = hal.switchs + i;
        file->ops.read = switch_read;
        file->ops.write = switch_write;
        file->ops.size = binary_size;
    }

    for (size_t i=0; i<hal.n_animations; i++){
        strcpy(path, "/animations/");
        strcat(path, hal.animations[i].name);
        strcat(path, "/frames");
        file = HALFS_insert(HALFS_root, path);
        file->backend = hal.animations + i;
        file->ops.write = animation_upload;

        strcpy(path, "/animations/");
        strcat(path, hal.animations[i].name);
        strcat(path, "/play");
        file = HALFS_insert(HALFS_root, path);
        file->backend = hal.animations + i;
        file->ops.read = anim_play_read;
        file->ops.write = anim_play_write;
        file->ops.size = binary_size;

        strcpy(path, "/animations/");
        strcat(path, hal.animations[i].name);
        strcat(path, "/loop");
        file = HALFS_insert(HALFS_root, path);
        file->backend = hal.animations + i;
        file->ops.read = anim_loop_read;
        file->ops.write = anim_loop_write;
        file->ops.size = binary_size;

        strcpy(path, "/animations/");
        strcat(path, hal.animations[i].name);
        strcat(path, "/fps");
        file = HALFS_insert(HALFS_root, path);
        file->backend = hal.animations + i;
        file->ops.read = anim_fps_read;
        file->ops.write = anim_fps_write;
        file->ops.size = anim_fps_size;
    }
}

/*
 * Detect arduino and launch I/O thread
 */
void * HALFS_init(struct fuse_conn_info *conn)
{
    glob_t globbuf;
    globbuf.gl_offs = 0;

    int flag = GLOB_DOOFFS;
    for(size_t i = 0; i < sizeof(ARDUINO_DEV_PATH)/sizeof(char *); i++){
        if (i == 1)
            flag = flag | GLOB_APPEND;
        glob(ARDUINO_DEV_PATH[i], flag, NULL, &globbuf);
    }
    printf("Found %lu possible arduinos in /dev/\n", (long unsigned int) globbuf.gl_pathc);
    for (size_t i = 0; i < globbuf.gl_pathc; i++){
        printf("Trying %s\n", globbuf.gl_pathv[i]);
        if (HAL_init(&hal, globbuf.gl_pathv[i]))
            break;
    }
    globfree(&globbuf);

    if (! hal.ready){
        fprintf(stderr, "Unable to find suitable arduino; force quit\n");
        exit(EXIT_FAILURE);
    }
    HAL_socket_open(&hal, "/tmp/hal.sock");
    HALFS_build();
    pthread_create(&com_thread, NULL, HAL_read_thread, &hal);
    return NULL;
}


/* ================= FUSE Operations ================= */
static int HALFS_open(const char *path, struct fuse_file_info *fi)
{
    HALFS *file = HALFS_find(HALFS_root, path);
    if (file)
        return 0;
    return -ENOENT;
}

static int HALFS_read(
    const char *path, 
    char *buf, 
    size_t size, 
    off_t offset,
    struct fuse_file_info *fi
){
    HALFS *file = HALFS_find(HALFS_root, path);
    int res = -ENOENT;
    if (file){
        res = file->ops.read(file->backend, buf, size, offset);
        printf("\033[1;35mREAD %s[%lu/%d]: \033[0m", path, size, res);
        for (int i=0; i<res; i++)
            printf("%02x", buf[i]);
        puts("");
    }
    return res;
}


static int HALFS_write(
    const char *path, 
    const char *buf, 
    size_t size, 
    off_t offset,
    struct fuse_file_info *fi
){
    HALFS *file = HALFS_find(HALFS_root, path);
    int res = -ENOENT;
    if (file){
        res = file->ops.write(file->backend, buf, size, offset);
        printf("\033[1;35mWRITE %s[%lu/%d]: \033[0m", path, size, res);
        for (int i=0; i<res; i++)
            printf("%02x", buf[i]);
        puts("");
    }
    return res;
}

static int HALFS_size(const char *path, struct fuse_file_info *fi)
{
    HALFS *file = HALFS_find(HALFS_root, path);
    if (file)
        return file->ops.size(file->backend);
    return -ENOENT;
}


static int HALFS_trunc(const char *path, off_t offset) 
{
    HALFS *file = HALFS_find(HALFS_root, path);
    if (file)
        return file->ops.trunc(file->backend);
    return -ENOENT;
}

static int HALFS_readdir(
    const char *path, 
    void *buf, 
    fuse_fill_dir_t filler,
    off_t offset, 
    struct fuse_file_info *fi
){

    HALFS *dir = HALFS_find(HALFS_root, path);
    if (! dir)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (HALFS *it=dir->first_child; it!=NULL; it=it->next_sibling)
        filler(buf, it->name, NULL, 0);

    return 0;
}

static int HALFS_readlink(const char *path, char *buf, size_t size)
{
    HALFS *file = HALFS_find(HALFS_root, path);
    if (file){
        if (file->ops.target == NULL)
            return -ENOENT;
        strcpy(buf, file->ops.target);
        return 0;
    }
    return -ENOENT;
}

static int HALFS_getattr(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));
    HALFS *file = HALFS_find(HALFS_root, path);
    if (! file)
        return -ENOENT;

    stbuf->st_uid = my_uid;
    stbuf->st_gid = my_gid;

    stbuf->st_mode = HALFS_mode(file);

    if (file->first_child){
        /* has child: Directory */
        stbuf->st_mode |= S_IFDIR;
        stbuf->st_nlink = 2;
    } else if (file->ops.target != NULL){
        /* has target: Symlink */
        stbuf->st_mode |= S_IFLNK;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(file->ops.target);
    } else {
        /* otherwise: Regular file */
        stbuf->st_mode |= S_IFREG;
        stbuf->st_nlink = 1;
        stbuf->st_size = file->ops.size(file->backend);
    }

    return 0;
}

static struct fuse_operations hal_ops = {
    .getattr    = HALFS_getattr,
    .readdir    = HALFS_readdir,
    .open       = HALFS_open,
    .read       = HALFS_read,
    .write      = HALFS_write,
    .truncate   = HALFS_trunc,
    .init       = HALFS_init,
    .readlink   = HALFS_readlink
};
/* ============================================== */

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &hal_ops, NULL);
}
