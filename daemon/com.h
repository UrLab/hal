#ifndef DEFINE_SHAREDQUEUE_HEADER
#define DEFINE_SHAREDQUEUE_HEADER

#include <stdbool.h>
#include "HALResource.h"

bool HAL_init(struct HAL_t *hal, const char *arduino_dev);

void HAL_socket_open(struct HAL_t *hal, const char *path);

void *HAL_read_thread(void *args);

void HAL_ask_trigger(struct HAL_t *hal, int trig_id);
void HAL_set_switch(struct HAL_t *hal, int switch_id, bool on);
void HAL_ask_switch(struct HAL_t *hal, int switch_id);

#endif