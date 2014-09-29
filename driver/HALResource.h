#ifndef DEFINE_HALRESOURCE_HEADER
#define DEFINE_HALRESOURCE_HEADER

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct HAL_t HAL_t;

/* Any of HAL components */
typedef struct HALResource_t {
    char *name;
    char type, id;
    HAL_t *hal;
    pthread_cond_t cond;
    union {
        void *ptr;
        unsigned char hhu4[4];
        bool b;
        float f;
    } data;
} HALResource;

#define HAL_SOCK_MAXCLIENTS 32

struct HAL_t {
    int serial_fd;
    int socket_fd;
    int socket_n_clients;
    int socket_clients[HAL_SOCK_MAXCLIENTS];

    size_t tx_bytes, rx_bytes;

    pthread_mutex_t mutex;
    char version[41];
    bool ready;

    /* Binary sensors (on/off) */
    size_t n_triggers;
    HALResource *triggers; 

    /* Analog sensors (0..1) */
    size_t n_sensors;
    HALResource *sensors;

    /* Binary outputs (on/off) */
    size_t n_switchs;
    HALResource *switchs;

    /* Analog outputs (sequences) */
    size_t n_animations;
    HALResource *animations;
};

static inline HALResource *HALResource_init(HALResource *res, const char *name, char type, char id, HAL_t *hal)
{
    res->name = strdup(name);
    res->type = type;
    res->id = id;
    res->hal = hal;
    pthread_cond_init(&res->cond, NULL);
    return res;
}

static inline void HALResource_destroy(HALResource *res)
{
    pthread_cond_destroy(&res->cond);
    free(res->name);
    memset(res, 0, sizeof(HALResource));
}

static inline void HALResource_destroyAll(size_t *n, HALResource *res)
{
    for (size_t i=0; i<*n; i++)
        HALResource_destroy(res+i);
    free(res);
    *n = 0;
}

#endif
