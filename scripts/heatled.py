#!/usr/bin/python

import hal
from time import sleep


def main():
    while True:
        temp = hal.sensor("temp_radiator")
        hal.upload("heater", hal.sinusoid(max_value=temp))
        sleep(10)


if __name__ == "__main__":
    main()
