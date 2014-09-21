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
void HAL_upload_anim(
    struct HAL_t *hal, 
    unsigned char anim_id, 
    unsigned char len, 
    const unsigned char *frames
);
void HAL_ask_anim_play(struct HAL_t *hal, int anim_id);
void HAL_set_anim_play(struct HAL_t *hal, int anim_id, bool play);
void HAL_ask_anim_loop(struct HAL_t *hal, int anim_id);
void HAL_set_anim_loop(struct HAL_t *hal, int anim_id, bool loop);
void HAL_ask_anim_delay(struct HAL_t *hal, int anim_id);
void HAL_set_anim_delay(struct HAL_t *hal, int anim_id, unsigned char delay);

void HAL_ask_sensor(struct HAL_t *hal, int sensor_id);

#endif