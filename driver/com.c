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
#include <sys/stat.h>

/* Establish communication with arduino && initialize HAL structure */
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

    /* Get HAL version */
    for (int k=0; k<5; k++){
        serialport_writebyte(hal->serial_fd, '?');
        n_try = 0;
        do {
            serialport_read_until(hal->serial_fd, buf, '\n', 128, 50);
            line = strip(buf);
            n_try++;
        } while (line[0] != '?' && n_try <= 20);
        if (line[0] == '?') break;
    }
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
            HALResource_init(resptr+i, line+1, type, i, hal);
        }
    }

    pthread_mutex_init(&hal->mutex, NULL);

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

/* Create && bind HAL events socket */
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
    chmod(sock_desc.sun_path, 0777);
}

/* Writes msg to every clients of HAL events socket */
void HAL_socket_write(struct HAL_t *hal, const char *msg)
{
    int len = (int) strlen(msg);
    for (int i=0; i<hal->socket_n_clients; i++){
        int client = hal->socket_clients[i];
        if (write(client, msg, len) != len){
            close(client);
            hal->socket_clients[i] = hal->socket_clients[hal->socket_n_clients-1];
            hal->socket_n_clients--;
        }
    }
}

/* Accept new client on HAL events socket; timeouts in 1ms */
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

/* Main input routine */
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

        hal->rx_bytes += strlen(buf);

        line = strip(buf);
        len = strlen(line);
        if (len == 0)
            continue;

        printf("\033[1;33m>> [%lu] %s\033[0m\n", len, line);

        cmd = line[0];

        /* PING/PONG (initiated by Arduino) */
        if (streq(line, "*"))
            serialport_writebyte(hal->serial_fd, '*');

        /* 
        * Trigger state change;
        * Trigger state response upon request;
        * Switch state 
        */
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

            pthread_mutex_lock(&(resource->hal->mutex));
            /* Update value */
            resource->data.b = (bool) val;

            /* Notify potential readers that the value is available */
            pthread_cond_signal(&(resource->cond));
            pthread_mutex_unlock(&(resource->hal->mutex));

            /* If state change, also write to socket */
            if (cmd == '!'){
                strcpy(buf, resource->name);
                strcat(buf, val ? ":1\n" : ":0\n");
                HAL_socket_write(hal, buf);
            }
        }

        /* Sensor value */
        else if (cmd == 'C'){
            HALResource *sensor = NULL;
            char *sep = strchr(line, ':');
            *sep = '\0';
            int id = strtol(line+1, NULL, 10);

            sensor = hal->sensors+id;
            sep++;

            int val = strtol(sep, NULL, 10);

            pthread_mutex_lock(&sensor->hal->mutex);
            sensor->data.f = ((float) val)/1023;
            pthread_cond_signal(&sensor->cond);
            pthread_mutex_unlock(&sensor->hal->mutex);
        }

        /* Animation attributes change */
        else if (cmd == 'a'){
            HALResource *anim = NULL;
            char *sep = strchr(line, ':');
            *sep = '\0';
            int id = strtol(line+1, NULL, 10);

            anim = hal->animations+id;
            sep++;

            pthread_mutex_lock(&(anim->hal->mutex));
            /* Update value */
            switch (*sep){
                case 'l': anim->data.hhu4[0] = (sep[1] == '1'); break;
                case 'p': anim->data.hhu4[1] = (sep[1] == '1'); break;
                case 'd': 
                    id = strtol(sep+1, NULL, 10);
                    anim->data.hhu4[2] = id;
            }

            /* Notify potential readers that the value is available */
            pthread_cond_signal(&(anim->cond));
            pthread_mutex_unlock(&(anim->hal->mutex));
        }

        else if (cmd == 'A'){
            HALResource *anim = NULL;
            char *sep = strchr(line, ':');
            *sep = '\0';
            int id = strtol(line+1, NULL, 10);
            anim = hal->animations+id;
            pthread_mutex_lock(&(anim->hal->mutex));
            pthread_cond_signal(&(anim->cond));
            pthread_mutex_unlock(&(anim->hal->mutex));
        }
    }
    return NULL;
}

typedef const unsigned char cuchar;

/* Send bytes on arduino serial link */
#define say(hal, char_array) HAL_say((hal), sizeof(char_array), (char_array))
static inline void HAL_say(struct HAL_t *hal, size_t len, cuchar *bytes)
{
    printf("\033[31;1m<< ");
    int j;
    for (size_t i=0; i<len; i++){
        if (('a'<=bytes[i] && bytes[i]<='z') || ('A'<=bytes[i] && bytes[i]<='Z') || ('0'<=bytes[i] && bytes[i]<='9'))
            putchar(bytes[i]);
        else
            printf("<%02hhx>", bytes[i]);            
        for (j=0; j<5; j++){
            if (serialport_writebyte(hal->serial_fd, bytes[i]) != 0)
                minisleep(0.001);
            else break;
        }
        if (j == 5)
            printf("\033[43m(ERR)\033[0;31;1m");
        else
            hal->tx_bytes++;
    }
    printf("\033[0m\n");
}

bool HAL_ask_trigger(HALResource *trigger)
{
    bool res = 0;
    cuchar req[] = {'T', trigger->id};

    pthread_mutex_lock(&trigger->hal->mutex);
    say(trigger->hal, req);
    pthread_cond_wait(&trigger->cond, &trigger->hal->mutex);
    res = trigger->data.b;
    pthread_mutex_unlock(&trigger->hal->mutex);

    return res;
}

void HAL_set_switch(HALResource *sw, bool on)
{
    cuchar req[] = {'S', sw->id, on ? 1 : 0};

    pthread_mutex_lock(&sw->hal->mutex);
    say(sw->hal, req);
    pthread_cond_wait(&sw->cond, &sw->hal->mutex);
    pthread_mutex_unlock(&sw->hal->mutex);
}

bool HAL_ask_switch(HALResource *sw)
{
    cuchar req[] = {'S', sw->id, 42};
    bool res = false;

    pthread_mutex_lock(&sw->hal->mutex);
    say(sw->hal, req);
    pthread_cond_wait(&sw->cond, &sw->hal->mutex);
    res = sw->data.b;
    pthread_mutex_unlock(&sw->hal->mutex);

    return res;
}

void HAL_upload_anim(
    HALResource *anim, 
    unsigned char len, 
    const unsigned char *frames
){
    cuchar req[258] = {'A', anim->id, len};
    memcpy((char*)req+3, frames, len);

    pthread_mutex_lock(&anim->hal->mutex);
    HAL_say(anim->hal, len+3, req);
    pthread_cond_wait(&anim->cond, &anim->hal->mutex);
    pthread_mutex_unlock(&anim->hal->mutex);
}

bool HAL_ask_anim_play(HALResource *anim)
{
    cuchar req[] = {'a', anim->id, 'p', 42};
    bool res = false;

    pthread_mutex_lock(&anim->hal->mutex);
    say(anim->hal, req);
    pthread_cond_wait(&anim->cond, &anim->hal->mutex);
    res = anim->data.hhu4[1];
    pthread_mutex_unlock(&anim->hal->mutex);

    return res;
}

void HAL_set_anim_play(HALResource *anim, bool play)
{
    cuchar req[] = {'a', anim->id, 'p', play ? 1 : 0};

    pthread_mutex_lock(&anim->hal->mutex);
    say(anim->hal, req);
    pthread_cond_wait(&anim->cond, &anim->hal->mutex);
    pthread_mutex_unlock(&anim->hal->mutex);
}

bool HAL_ask_anim_loop(HALResource *anim)
{
    cuchar req[] = {'a', anim->id, 'l', 42};
    bool res = false;

    pthread_mutex_lock(&anim->hal->mutex);
    say(anim->hal, req);
    pthread_cond_wait(&anim->cond, &anim->hal->mutex);
    res = anim->data.hhu4[0];
    pthread_mutex_unlock(&anim->hal->mutex);

    return res;
}

void HAL_set_anim_loop(HALResource *anim, bool loop)
{
    cuchar req[] = {'a', anim->id, 'l', loop ? 1 : 0};

    pthread_mutex_lock(&anim->hal->mutex);
    say(anim->hal, req);
    pthread_cond_wait(&anim->cond, &anim->hal->mutex);
    pthread_mutex_unlock(&anim->hal->mutex);
}

unsigned char HAL_ask_anim_delay(HALResource *anim)
{
    cuchar req[] = {'a', anim->id, 'd', 0};
    unsigned char res = false;

    pthread_mutex_lock(&anim->hal->mutex);
    say(anim->hal, req);
    pthread_cond_wait(&anim->cond, &anim->hal->mutex);
    res = anim->data.hhu4[2];
    pthread_mutex_unlock(&anim->hal->mutex);

    return res;
}

void HAL_set_anim_delay(HALResource *anim, unsigned char delay)
{
    cuchar req[] = {'a', anim->id, 'd', delay};

    pthread_mutex_lock(&anim->hal->mutex);
    say(anim->hal, req);
    pthread_cond_wait(&anim->cond, &anim->hal->mutex);
    pthread_mutex_unlock(&anim->hal->mutex);
}

float HAL_ask_sensor(HALResource *sensor)
{
    cuchar req[] = {'C', sensor->id};
    float res = 0;

    pthread_mutex_lock(&sensor->hal->mutex);
    say(sensor->hal, req);
    pthread_cond_wait(&sensor->cond, &sensor->hal->mutex);
    res = sensor->data.f;
    pthread_mutex_unlock(&sensor->hal->mutex);

    return res;
}

size_t HAL_rx_bytes(struct HAL_t *hal)
{
    size_t res = 0;
    pthread_mutex_lock(&hal->mutex);
    res = hal->rx_bytes;
    pthread_mutex_unlock(&hal->mutex);
    return res;
}

size_t HAL_tx_bytes(struct HAL_t *hal)
{
    size_t res = 0;
    pthread_mutex_lock(&hal->mutex);
    res = hal->tx_bytes;
    pthread_mutex_unlock(&hal->mutex);
    return res;
}
