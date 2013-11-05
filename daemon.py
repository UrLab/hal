from ambianceduino import Ambianceduino, AmbianceduinoNotFound
from urllib import urlopen
from time import sleep
from math import log
from datetime import datetime
import json
import puka
from math import log

import logging

from config import *

# create LOG
LOG = logging.getLogger(__name__)
LOG.setLevel(logging.INFO)
handler = logging.FileHandler('daemon.log')
handler.setLevel(logging.DEBUG)
handler.setFormatter(logging.Formatter('[%(asctime)s] <%(levelname)s> %(name)s: %(message)s'))
LOG.addHandler(handler)

def avg_for_key(list_of_dictionaries, key):
	"""
	[{'a': 1}, {'a': 5}], 'a' # => 3
	"""
	return sum([x[key] for x in list_of_dictionaries])/len(list_of_dictionaries)

def convertThermistor(analogValue, resistance, beta):
	vout = 5*analogValue/1023.0
	r = resistance*(5-vout)/vout
	return (beta/(log(r/resistance)+beta/298.15))-273.15

class AmbianceDaemon(Ambianceduino):

	def __init__(self, *args, **kwargs):
		super(AmbianceDaemon, self).__init__(*args, **kwargs)
		self.current_delay = None
		self.powered = None
		self.meteo = []
		self.__init_puka_client()

	def __init_puka_client(self):
		try:
			self.puka_client = puka.Client(AMQ_SERVER)
			promise = self.puka_client.connect()
			self.puka_client.wait(promise)
			LOG.info("Connected AMQ to %s"%(AMQ_SERVER))

			promise = self.puka_client.queue_declare(queue=METEO_QUEUE)
			self.puka_client.wait(promise)
			LOG.info("Got queue %s"%(METEO_QUEUE))
		except:
			LOG.error("Unable to connect to %s#%s"%(AMQ_SERVER, METEO_QUEUE))
			self.puka_client = None

	def default_handler(self, *args):
		LOG.info(' '.join(map(str, args)))

	def when_on(self):
		self.powered = True
		LOG.info('Powered on')

	def when_off(self):
		self.powered = False
		LOG.info('Powered off')

	def when_delay(self, delay):
		self.current_delay = delay
		LOG.info('Delay set to %d'%(delay))

	def when_bell(self):
		LOG.info('Someone rings the bell')

	def when_analogs(self, analogs):
		if not self.puka_client:
			return 
		self.meteo.append(analogs)
		if len(self.meteo) == METEO_LEN:
			msg = {
				'time': str(datetime.now()),
				'light' : {
					'inside': avg_for_key(self.meteo, 'light_in')/10.23,
					'outside': avg_for_key(self.meteo, 'light_out')/10.23
				},
				'temperature' : {
					'radiator': convertThermistor(avg_for_key(self.meteo, 'temp_radia'), 4700, 3950),
					'ambiant': convertThermistor(avg_for_key(self.meteo, 'temp_amb'), 2200, 3950)
				}
			}
			promise = self.puka_client.basic_publish(
				exchange='', 
				routing_key=METEO_QUEUE,
				body=json.dumps(msg)
			)
			self.meteo = []
			self.puka_client.wait(promise)
			LOG.info('Sent analog values')


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
			d = int(25/log(people+2))
			if d != self.current_delay:
				self.delay(d)
		except Exception as err:
			LOG.error("%s: %s"%(err.__class__.__name__, err.message))

	def mainloop(self):
		while True:
			self.analogs()
			sleep(10)
			self.spacestatus()
			self.peoplecount()
			self.analogs()
			sleep(10)

if __name__ == "__main__":
	try:
		a = AmbianceDaemon(boot_time=10)
	except Exception as err:
		LOG.error("%s: %s"%(err.__class__.__name__, err))
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

