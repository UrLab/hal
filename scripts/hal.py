# HAL API
# Use the functions get() and set() to access driver values. These two functions 
# do the right cast and you'll get a usable value instead of a string.

from math import sin, pi
from config import HALFS_ROOT
import socket

def read(filename):
	""" Returns a string with the content of the file given in parameter """
	return open(HALFS_ROOT + ("/" if filename[0] != "/" else "") + filename, 'r').read().strip("\0").strip()

def write(filename, value):
	""" Casts value to str and writes it to the file given in parameter """
	open(HALFS_ROOT + ("/" if filename[0] != "/" else "") + filename, 'w').write(str(value))

def get(filename):
	""" Returns float or int value depending on the type of the value in the file """
	str_value = read(filename)
	if str_value.find(".") == -1: # value is an integer
		return int(str_value)
	else: # value is a float
		return float(str_value)
	return 0

def set(filename, value):
	""" Casts value to the correct type and writes it to the file given in parameter """
	if type(value) == bool:
		value = int(value)
	if read(filename).find(".") == -1: # value to write is an integer
		write(filename, int(value))
	else: # value to write is a float
		write(filename, float(value))

def events():
	events = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
	events.connect(HALFS_ROOT+"/events")
	while True:
	    line = events.recv(16)
	    line = line.strip()
	    trig_name, state = line.split(':')
	    yield trig_name, (state == '1')

def sinusoid(n_frames, val_min, val_max):
    assert val_min <= val_max
    for n in n_frames, val_min, val_max:
		assert 0 <= n < 256

    a = (val_max-val_min)/2
    m = val_min+a
    return ''.join([chr(int(m + a*sin(2*i*pi/n_frames))) for i in range(n_frames)])

