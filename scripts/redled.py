#!/usr/bin/python

from urllib2 import urlopen
from json import loads
from config import HALFS_ROOT
from time import sleep

macs_json = urlopen("http://pamela.urlab.be/mac.json").read()
known_people = 0

if macs_json:
	macs = loads(macs_json)
	known_people = len(macs["color"])

if __name__ == "__main__" :
	# If number of people has changed, epileptic mode for 2 seconds
	if (int(open(HALFS_ROOT+"/animations/red/fps", 'r').read().strip())/10 != known_people or True):
		open(HALFS_ROOT+"/animations/red/fps", 'w').write(str(500))
		sleep(2)
	open(HALFS_ROOT+"/animations/red/fps", 'w').write(str(10*known_people))



