#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "arduino-serial-lib.h"
#include "Ambianceduino.h"
#include "utils.h"


void __protected_HAL_write(HAL *hal, size_t offset, void *elem, size_t elem_size)
{
    if (HAL_lock(hal)){
        char *dest = ((char *) hal) + offset;
        memcpy(dest, elem, elem_size);
        HAL_unlock(hal);
    }
}

void __protected_HAL_read(HAL *hal, size_t offset, void *elem, size_t elem_size)
{
    if (HAL_lock(hal)){
        char *src = ((char *) hal) + offset;
        memcpy(elem, src, elem_size);
        HAL_unlock(hal);
    }
}

bool HAL_init(HAL *hal, const char *serial_port, int bauds)
{
    memset(hal, 0, sizeof(HAL));
    int fd = serialport_init(serial_port, bauds);
    if (fd >= 0){
        hal->__fd = fd;
        pthread_mutex_init(&(hal->__mutex), NULL);
        sleep(3); /* Arduino boot time */
        return true;
    }
    return false;
}

void HAL_destroy(HAL *hal)
{
    pthread_mutex_destroy(&(hal->__mutex));
    serialport_close(hal->__fd);
}

void HAL_askAnalog(HAL *hal, unsigned char sensor_id)
{
    if (sensor_id < 6){
        serialport_writebyte(hal->__fd, '@');
        serialport_writebyte(hal->__fd, sensor_id);
        sleep(1);
    }
}

void HAL_on(HAL *hal)
{
    serialport_writebyte(hal->__fd, '-');
    sleep(1);
}

void HAL_off(HAL *hal)
{
    serialport_writebyte(hal->__fd, '_');
    sleep(1);
}

/* === Parsers === */
static bool HAL_parseState(HAL *hal, const char *command)
{
    HALState read_state = STANDBY;
    if (streq(command, "POWERED"))
        read_state = POWERED;
    else if (streq(command, "SHOWTIME"))
        read_state = SHOWTIME;
    else if (streq(command, "ALERT"))
        read_state = ALERT;

    HAL_WRITE(hal, state, read_state);
    return true;
}

static bool HAL_parseSensors(HAL *hal, char *command)
{
    char *split = strchr(command, ':');
    if (split){
        int val = strtol(split+1, NULL, 10);
        *split = '\0';

        if (streq(command, "light_in"))
            HAL_WRITE(hal, light_inside, val);

        else if (streq(command, "light_out"))
            HAL_WRITE(hal, light_outside, val);

        else if (streq(command, "temp_amb"))
            HAL_WRITE(hal, temp_ambiant, val);

        else if (streq(command, "temp_radia"))
            HAL_WRITE(hal, temp_radiator, val);
    }
    return true;
}

static bool HAL_parseTrigger(HAL *hal, char *command)
{
    size_t l = strlen(command);
    if (l > 1){
        bool state = command[l-1] == '1';
        command[l-1] = '\0';

        if (streq(command, "bell"))
            HAL_WRITE(hal, bell, state);

        else if (streq(command, "passage"))
            HAL_WRITE(hal, passage, state);

        else if (streq(command, "door"))
            HAL_WRITE(hal, door, state);

        else if (streq(command, "radiator"))
            HAL_WRITE(hal, radiator, state);
    }

    return true;
}

static bool HAL_parse(HAL *hal, char *command)
{
    //printf("PARSE \"%s\"\n", command);
    bool power_on;
    switch (command[0]){
        case '-': 
            power_on = true; 
            HAL_WRITE(hal, on, power_on);
            return true;
        case '_': 
            power_on = false; 
            HAL_WRITE(hal, on, power_on);
            return true;
        case '?':
            strncpy(hal->version, command+1, 40);
            return true;

        case 'S':
            return HAL_parseState(hal, command+1);
        case '@':
            return HAL_parseSensors(hal, command+1);
        case 'T':
            return HAL_parseTrigger(hal, command+1);

        default:
            break;
    }
    return false;
}

/*! Return true if a message was coreectly processed, false otherwise */
bool HAL_getMsg(HAL *hal)
{
    char buf[256];
    if (serialport_read_until(hal->__fd, buf, '\n', sizeof(buf), 1500))
        return HAL_parse(hal, strip(buf));
    return false;
}

static void *HAL_readerThread(void *hal_as_void)
{
    HAL *hal = (HAL *) hal_as_void;
    bool running;
    HAL_READ(hal, __running, running);

    while (running){
        HAL_getMsg(hal);
        HAL_READ(hal, __running, running);
    }

    pthread_exit(NULL);
}

void HAL_start(HAL *hal)
{
    if (HAL_lock(hal)){
        if (! hal->__running){
            hal->__running = true;
            pthread_create(&(hal->__reader_thread), NULL, HAL_readerThread, hal);
        }
        HAL_unlock(hal);
    }
}

void HAL_stop(HAL *hal)
{
    bool running;
    HAL_READ(hal, __running, running);
    if (running){
        running = false;
        HAL_WRITE(hal, __running, running);    
        void *ret;
        pthread_join(hal->__reader_thread, &ret);
    }
}
