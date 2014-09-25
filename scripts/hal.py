# HAL API
# Use the functions get() and set() to access driver values. These two functions 
# do the right cast and you'll get a usable value instead of a string.

from config import HALFS_ROOT

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
	if read(filename).find(".") == -1: # value to write is an integer
		write(filename, int(value))
	else: # value to write is a float
		write(filename, float(value))
