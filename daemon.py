from ambianceduino import Ambianceduino, AmbianceduinoNotFound
from urllib import urlopen
from time import sleep
from math import log
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
		super(AmbianceDaemon, self).__init__(*args, **kwargs)
		self.powered = None

	def default_handler(self, *args):
		LOG.info(' '.join(map(str, args)))

	def when_on(self):
		self.powered = True
		LOG.info('Powered on')

	def when_off(self):
		self.powered = False
		LOG.info('Powered off')

	def when_delay(self, delay):
		LOG.info('Delay set to %d'%(delay))

	def when_bell(self):
		LOG.info('Someone rings the bell')

	def spacestatus(self):
		try:
			statuspage = urlopen('http://api.urlab.be/spaceapi/status')
			payload = json.loads(statuspage.read())
			if 'state' in payload:
				if payload['state'] == 'open' and self.powered != True:
					self.on()
				elif payload['state'] == 'closed' and self.powered != False:
					self.off()
		except Exception as err:
			LOG.error("%s: %s"%(err.__class__.__name__, err.message))

	def peoplecount(self):
		try:
			pamelapage = urlopen('http://pamela.urlab.be/mac.json')
			payload = json.loads(pamelapage.read())
			people = len(payload['color']) + len(payload['grey'])
			self.delay(int(25/log(people+2)))
		except Exception as err:
			LOG.error("%s: %s"%(err.__class__.__name__, err.message))

	def mainloop(self):
		while True:
			self.spacestatus()
			self.peoplecount()
			sleep(20)

if __name__ == "__main__":
	try:
		a = AmbianceDaemon(boot_time=10)
	except Exception as err:
		LOG.error("%s: %s"%(err.__class__.__name__, err.message))
		exit(1)

	LOG.info('Got Ambianceduino')

	try:
		a.run()
		a.mainloop()
	except KeyboardInterrupt:
		a.stop()
		LOG.info('Stopping')
		exit()
	except Exception as err:
		LOG.error("%s: %s"%(err.__class__.__name__, err.message))

