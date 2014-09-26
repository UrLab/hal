#!/usr/bin/python

import hal
from time import sleep


def main():
    last_temp = -1
    hal.play("heater")
    hal.fps("heater", 42)
    while True:
        temp = hal.sensor("temp_radiator")
        if last_temp != temp:
            hal.upload("heater", hal.sinusoid(val_max=temp, n_frames=100))
            last_temp = temp
        sleep(10)


if __name__ == "__main__":
    main()
