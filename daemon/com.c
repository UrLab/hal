#include "arduino-serial-lib.h"
#include "HALResource.h"
#include "utils.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/time.h>

bool HAL_init(struct HAL_t *hal, const char *arduino_dev)
{
    char buf[128];
    char *line;
    char type;
    long int n_items;
    int n_try;
    HALResource *resptr = NULL;

    memset(hal, 0, sizeof(struct HAL_t));
    hal->serial_fd = serialport_init(arduino_dev, 115200);
    if (hal->serial_fd < 0)
        return false;

    minisleep(5.0);

    /* Get HAL version */
    serialport_writebyte(hal->serial_fd, '?');
    n_try = 0;
    do {
        serialport_read_until(hal->serial_fd, buf, '\n', 128, 1000);
        line = strip(buf);
        n_try++;
    } while (line[0] != '?' && n_try <= 20);
    if (line[0] != '?')
        goto fail;

    strncpy(hal->version, line+1, 40);
    hal->version[40] = '\0';

    /* Get HAL structure (Resources) */
    serialport_writebyte(hal->serial_fd, '$');
    for (int i=0; i<4; i++){
        serialport_read_until(hal->serial_fd, buf, '\n', 128, 1000);
        line = strip(buf);
        if (line[0] != '$')
            goto fail;

        type = line[1];
        n_items = strtol(line+2, NULL, 10);

        switch (type){
            case 'T':
                hal->triggers = calloc(n_items, sizeof(HALResource));
                hal->n_triggers = n_items;
                resptr = hal->triggers;
                break;
            case 'C':
                hal->sensors = calloc(n_items, sizeof(HALResource));
                hal->n_sensors = n_items;
                resptr = hal->sensors;
                break;
            case 'S':
                hal->switchs = calloc(n_items, sizeof(HALResource));
                hal->n_switchs = n_items;
                resptr = hal->switchs;
                break;
            case 'A':
                hal->animations = calloc(n_items, sizeof(HALResource));
                hal->n_animations = n_items;
                resptr = hal->animations;
                break;
            default:
                goto fail;
        }

        for (int i=0; i<n_items; i++){
            serialport_read_until(hal->serial_fd, buf, '\n', 128, 1000);
            line = strip(buf);
            if (line[0] != '$')
                goto fail;
            HALResource_init(resptr+i, line+1, type, i);
        }
    }

    hal->ready = true;
    return true;

    fail:
        if (hal->n_triggers) HALResource_destroyAll(&hal->n_triggers, hal->triggers);
        if (hal->n_sensors) HALResource_destroyAll(&hal->n_sensors, hal->sensors);
        if (hal->n_switchs) HALResource_destroyAll(&hal->n_switchs, hal->switchs);
        if (hal->n_animations) HALResource_destroyAll(&hal->n_animations, hal->animations);
        serialport_close(hal->serial_fd);
        memset(hal, 0, sizeof(struct HAL_t));
        return false;
}

void HAL_socket_open(struct HAL_t *hal, const char *path)
{
    size_t len;
    struct sockaddr_un sock_desc;
    hal->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    sock_desc.sun_family = AF_UNIX;
    strcpy(sock_desc.sun_path, path);
    unlink(sock_desc.sun_path);
    
    len = strlen(sock_desc.sun_path) + sizeof(sock_desc.sun_family);
    bind(hal->socket_fd, (struct sockaddr *)&sock_desc, len);
    listen(hal->socket_fd, HAL_SOCK_MAXCLIENTS);
}

void HAL_socket_write(struct HAL_t *hal, const char *msg)
{
    int len = (int) strlen(msg) + 1;
    for (int i=0; i<hal->socket_n_clients; i++){
        int client = hal->socket_clients[i];
        if (write(client, msg, len) != len){
            close(client);
            hal->socket_clients[i] = hal->socket_clients[hal->socket_n_clients-1];
            hal->socket_n_clients--;
        }
    }
}

static void HAL_socket_accept(struct HAL_t *hal)
{
    fd_set set;
    struct timeval timeout = {.tv_sec = 0, .tv_usec = 1000};
    FD_SET(hal->socket_fd, &set);
    select(hal->socket_fd+1, &set, NULL, NULL, &timeout);
    if (! FD_ISSET(hal->socket_fd, &set))
        return;

    int client = accept(hal->socket_fd, NULL, NULL);
    hal->socket_clients[hal->socket_n_clients] = client;
    hal->socket_n_clients++;
}

void *HAL_read_thread(void *args)
{
    char buf[128];
    char *line, cmd;
    size_t len;
    int val, i;

    struct HAL_t *hal = (struct HAL_t *) args;

    while (true){
        HAL_socket_accept(hal);

        if (! serialport_read_until(hal->serial_fd, buf, '\n', sizeof(buf), 100))
            continue;

        line = strip(buf);
        len = strlen(line);

        printf("\033[1;33m>> [%lu] %s\033[0m\n", len, line);

        cmd = line[0];

        /* PING/PONG (initiated by Arduino) */
        if (streq(line, "*"))
            serialport_writebyte(hal->serial_fd, '*');

        /* Trigger state change or status response upon request */
        else if (cmd == 'T' || cmd == '!' || cmd == 'S'){
            HALResource *resource = NULL;

            val = line[len-1] == '1';
            line[len-1] = '\0';
            i = strtol(line+1, NULL, 10);
            
            switch (cmd){
                case 'T':
                case '!': resource = hal->triggers+i; break;
                case 'S': resource = hal->switchs+i; break;
            }

            pthread_mutex_lock(&(resource->mutex));
            /* Update value */
            bool *valptr = (bool*) &resource->data;
            *valptr = (bool) val;

            /* Notify potential readers that the value is available */
            pthread_cond_broadcast(&(resource->cond));
            pthread_mutex_unlock(&(resource->mutex));

            /* If state change, also write to socket */
            if (cmd == '!'){
                strcpy(buf, resource->name);
                strcat(buf, val ? ":1\n" : ":0\n");
                HAL_socket_write(hal, buf);
            }
        }
    }
    return NULL;
}

void HAL_ask_trigger(struct HAL_t *hal, int trig_id)
{
    serialport_writebyte(hal->serial_fd, 'T');
    serialport_writebyte(hal->serial_fd, trig_id);
    printf("\033[31mT%d\033[0m\n", trig_id);
}

void HAL_set_switch(struct HAL_t *hal, int switch_id, bool on)
{
    serialport_writebyte(hal->serial_fd, 'S');
    serialport_writebyte(hal->serial_fd, switch_id);
    serialport_writebyte(hal->serial_fd, on ? 1 : 0);
    printf("\033[31mS%d%d\033[0m\n", switch_id, on ? 1 : 0);
}

void HAL_ask_switch(struct HAL_t *hal, int switch_id)
{
    serialport_writebyte(hal->serial_fd, 'S');
    serialport_writebyte(hal->serial_fd, switch_id);
    serialport_writebyte(hal->serial_fd, 42);
    printf("\033[31mS%d%d\033[0m\n", switch_id, 42);
}

void HAL_upload_anim(
    struct HAL_t *hal, 
    unsigned char anim_id, 
    unsigned char len, 
    const unsigned char *frames
){
    serialport_writebyte(hal->serial_fd, 'A');
    serialport_writebyte(hal->serial_fd, anim_id);
    serialport_writebyte(hal->serial_fd, len);
    for (unsigned char i=0; i<len; i++){
        if (serialport_writebyte(hal->serial_fd, frames[i]))
            printf("\033[31;1mERREUR %hhu !!!\033[0m\n", i);
        if (i%10 == 0)
            minisleep(0.001);
    }
    printf("\033[31mA%hhu:%hhu\033[0m\n", anim_id, len);
}
