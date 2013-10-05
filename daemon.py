from controller import *
from urllib import urlopen

import json

class AmbianceDaemon(Ambianceduino):
	def spacestatus(self):
		statuspage = urlopen('http://api.urlab.be/spaceapi/status')
		payload = json.loads(statuspage)
		if 'state' in payload:
			if payload['state'] == 'open':
				self.on()
			else:
				self.off()

	def mainloop(self):
		while True:
			self.sleep(60)
			self.spacestatus()

if __name__ == "__main__":
	AmbianceDaemon(boot_time=5).mainloop()
