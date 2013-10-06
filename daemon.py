from controller import *
from urllib import urlopen

import json

class AmbianceDaemon(Ambianceduino):
	def spacestatus(self):
		statuspage = urlopen('http://api.urlab.be/spaceapi/status')
		payload = json.loads(statuspage.read())
		if 'state' in payload:
			if payload['state'] == 'open':
				self.on()
				print "ON"
			else:
				self.off()
				print "OFF"

	def mainloop(self):
		while True:
			self.spacestatus()
			sleep(60)

if __name__ == "__main__":
	a = AmbianceDaemon(boot_time=10)
	print "Got Ambianceduino"
	a.mainloop()
