#ifndef DEFINE_AMBIANCEDUINO_C_HEADER
#define DEFINE_AMBIANCEDUINO_C_HEADER

#include <pthread.h>
#include <stdbool.h>

typedef enum {STANDBY=0, POWERED=1, SHOWTIME=2, ALERT=3} HALState;

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

/*! Open serial port and initialize HAL structure */
bool HAL_init(HAL *hal, const char *serial_port, int bauds);
/*! Destroy all elements in HAL structure */
void HAL_destroy(HAL *hal);

/*! Return true if a message has been popped and processed */
bool HAL_getMsg(HAL *hal);


/* === Requests === */
/*! Ask an analog value to the arduino */
void HAL_askAnalog(HAL *hal, unsigned char sensor_id);

/*! Ask Ambianceduino version */
void HAL_askVersion(HAL *hal);

/*! Set power on/off */
void HAL_on(HAL *hal);
void HAL_off(HAL *hal);


/* === Multithread-related procedures === */
/*! Start reader thread in background */
void HAL_start(HAL *hal);
/*! Stop and join reader thread */
void HAL_stop(HAL *hal);

/*! Acquire lock on HAL structure (should be released) */
static inline bool HAL_lock(HAL *hal){
	return pthread_mutex_lock(&(hal->__mutex)) == 0;
}

/*! Release lock on HAL structure */
static inline bool HAL_unlock(HAL *hal){
	return pthread_mutex_unlock(&(hal->__mutex)) == 0;
}


/*! Return the offset of field in strukt */
#define offsetof(strukt, field) (size_t) &(((strukt*)NULL)->field)

/*! Acquire lock on HAL structure, copy data into struct then release lock 
 *  Equivalent to 
 *      hal.lock();
 *      hal.field = val;
 *      hal.unlock()
 */
#define HAL_WRITE(hal, field, val) \
    __protected_HAL_write(hal, offsetof(HAL, field), &val, sizeof(val))
void __protected_HAL_write(HAL *hal, size_t offset, void *src, size_t src_size);


/*! Acquire lock on HAL structure, copy data from struct then release lock 
 *  Equivalent to 
 *      hal.lock();
 *      val = hal.field;
 *      hal.unlock()
 */
#define HAL_READ(hal, field, dest) \
    __protected_HAL_read(hal, offsetof(HAL, field), &dest, sizeof(dest))

void __protected_HAL_read(HAL *hal, size_t offset, void *dst, size_t dst_size);
#endif