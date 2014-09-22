#!/usr/bin/python

from config import HALFS_ROOT
from time import sleep

def limit(value, lower, upper):
	return max(lower, min(upper, value))

if __name__ == "__main__":
	while True:
		capted_temp = float(open(HALFS_ROOT+"/sensors/temp_radiator").read().strip("\0"))
		transformed_temp = 255*capted_temp
		open(HALFS_ROOT+"/animations/heater/frames").write(chr(limit(transformed_temp, 0, 255)))
		sleep(1)