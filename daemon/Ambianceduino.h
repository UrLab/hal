#ifndef DEFINE_AMBIANCEDUINO_C_HEADER
#define DEFINE_AMBIANCEDUINO_C_HEADER

#include <pthread.h>
#include <stdbool.h>

typedef enum {STANDBY, POWERED, SHOWTIME, ALERT} HALState;

typedef struct HAL_t {
	/* System */
	int __fd;
	pthread_mutex_t __mutex;
	pthread_t __reader_thread;
	bool __running;

	/* Control */
	char version[41]; //!< Arduino running code version (git hash)
	HALState state;

	/* Analog sensors */
	int temp_ambiant;
	int temp_radiator;
	int light_inside;
	int light_outside;

	/* Digital sensors */
	bool door;     //!< Stairs door
	bool bell;     //!< Main entrance bell
	bool radiator; //!< Radiator on/off
	bool passage;  //!< Passage between main room and CIEE
	bool on;       //!< Power on/off
} HAL;

bool HAL_init(HAL *hal, const char *serial_port, int bauds);

void HAL_destroy(HAL *hal);

void HAL_askAnalog(HAL *hal, unsigned char sensor_id);

void HAL_on(HAL *hal);
void HAL_off(HAL *hal);

bool HAL_getMsg(HAL *hal);

void HAL_start(HAL *hal);
void HAL_stop(HAL *hal);

static inline bool HAL_lock(HAL *hal){
	return pthread_mutex_lock(&(hal->__mutex)) == 0;
}

static inline bool HAL_unlock(HAL *hal){
	return pthread_mutex_unlock(&(hal->__mutex)) == 0;
}

#define offsetof(strukt, field) (size_t) &(((strukt*)NULL)->field)

#define HAL_WRITE(hal, field, val) \
    __protected_HAL_write(hal, offsetof(HAL, field), &val, sizeof(val))
void __protected_HAL_write(HAL *hal, size_t offset, void *src, size_t src_size);

#define HAL_READ(hal, field, dest) \
    __protected_HAL_read(hal, offsetof(HAL, field), &dest, sizeof(dest))

void __protected_HAL_read(HAL *hal, size_t offset, void *dst, size_t dst_size);
#endif