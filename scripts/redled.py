#!/usr/bin/python

from urllib2 import urlopen
from json import loads
from config import HALFS_ROOT

#macs_json = urlopen("http://pamela.urlab.be/mac.json").read()
macs_json='{"color": ["hyperflow", "rom1", "staz", "SupayrPoney", "piR", "Titou"], "hidden": 2, "grey": ["xx:xx:xx:xx:4e:f9"]}'
macs = loads(macs_json)

known_people = len(macs["color"])

if __name__ = "__main__":
	open(HALFS_ROOT+"/animations/red/fps", 'w').write(str(10*known_people))



