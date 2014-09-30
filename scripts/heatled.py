#!/usr/bin/python

import hal
from time import sleep

logger = hal.getLogger(__name__)

def main():
    last_temp = -1
    is_playing = False

    hal.fps("heater", 42)
    while True:
        if hal.trig("heater"):
            if not is_playing:
                is_playing = True
            temp = hal.sensor("temp_radiator")
            if last_temp != temp:
                hal.upload("heater", hal.sinusoid(val_max=temp, n_frames=100))
                last_temp = temp
                logger.info("Uploading new heater sinusoid for value %f" % (temp))
        elif is_playing:
            hal.stop("heater")
            is_playing = False
        sleep(60)


if __name__ == "__main__":
    main()
