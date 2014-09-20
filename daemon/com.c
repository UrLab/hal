#include "arduino-serial-lib.h"
#include "HALResource.h"
#include <assert.h>
#include "utils.h"

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
    } while (line[0] != '?' && n_try <= 10);
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

void *HAL_read_thread(void *args)
{
    return NULL;
}
