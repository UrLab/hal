#ifndef DEFINE_SHAREDQUEUE_HEADER
#define DEFINE_SHAREDQUEUE_HEADER

#include <stdbool.h>
#include "HALResource.h"

bool HAL_init(struct HAL_t *hal, const char *arduino_dev);

void *HAL_read_thread(void *args);

#endif