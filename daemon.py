from ambianceduino import Ambianceduino, AmbianceduinoNotFound
from urllib import urlopen
from time import sleep
import json

import logging

# create LOG
LOG = logging.getLogger(__name__)
LOG.setLevel(logging.INFO)
handler = logging.FileHandler('daemon.log')
handler.setLevel(logging.DEBUG)
handler.setFormatter(logging.Formatter('[%(asctime)s] <%(levelname)s> %(name)s: %(message)s'))
LOG.addHandler(handler)

class AmbianceDaemon(Ambianceduino):
	def __init__(self, *args, **kwargs):
		Ambianceduino.__init__(self, *args, **kwargs)
		self.powered = None

	def default_handler(self, *args):
		LOG.info(' '.join(map(str, args)))

	def when_on(self):
		self.powered = True
		LOG.info('Powered on')

	def when_off(self):
		self.powered = False
		LOG.info('Powered off')

	def spacestatus(self):
		try:
			statuspage = urlopen('http://api.urlab.be/spaceapi/status')
			payload = json.loads(statuspage.read())
			if 'state' in payload:
				if payload['state'] == 'open' and self.powered != True:
					self.on()
				elif self.powered != False:
					self.off()
		except Exception as err:
			LOG.error("%s: %s"%(err.__class__.__name__, err.message))

	def mainloop(self):
		while True:
			self.spacestatus()
			sleep(20)

if __name__ == "__main__":
	a = AmbianceDaemon(boot_time=10)
	LOG.info('Got Ambianceduino')

	try:
		a.run()
		a.mainloop()
	except Exception as err:
		a.stop()
		LOG.error("%s: %s"%(err.__class__.__name__, err.message))
