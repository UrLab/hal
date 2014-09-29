#ifndef DEFINE_SHAREDQUEUE_HEADER
#define DEFINE_SHAREDQUEUE_HEADER

#include <stdbool.h>
#include "HALResource.h"

bool HAL_init(struct HAL_t *hal, const char *arduino_dev);

void HAL_socket_open(struct HAL_t *hal, const char *path);

void *HAL_read_thread(void *args);

bool HAL_ask_trigger(HALResource *trigger);

void HAL_set_switch(HALResource *sw, bool on);
bool HAL_ask_switch(HALResource *sw);

void HAL_upload_anim(
    HALResource *anim, 
    unsigned char len, 
    const unsigned char *frames
);
bool HAL_ask_anim_play(HALResource *anim);
void HAL_set_anim_play(HALResource *anim, bool play);
bool HAL_ask_anim_loop(HALResource *anim);
void HAL_set_anim_loop(HALResource *anim, bool loop);
unsigned char HAL_ask_anim_delay(HALResource *anim);
void HAL_set_anim_delay(HALResource *anim, unsigned char delay);

float HAL_ask_sensor(HALResource *sensor);

size_t HAL_rx_bytes(struct HAL_t *hal);
size_t HAL_tx_bytes(struct HAL_t *hal);

#endif