from serial import Serial
from glob import glob
from time import sleep
import json

class AmbianceduinoNotFound(Exception):
	pass

class Ambianceduino:
	DEV_PATTERNS = ["/dev/ttyACM*", "/dev/ttyUSB*"]

	def __init__(self, max_try=5):
		possible_devices = [f for pattern in self.DEV_PATTERNS for f in glob(pattern)]
		for device in possible_devices:
			self.serial = Serial(device, 115200, timeout=1)
			sleep(3) #Wait arduino boot
			self.serial.write('?')
			got = self.serial.readline()
			#Magic string
			if "jesuisuncanapequichante" in got:
				break
			else:
				print(got)
				self.serial.close()
				self.serial = None
		if not self.serial:
			raise AmbianceduinoNotFound(str(possible_devices))

	def get_animation_delay(self):
		self.serial.write('#')
		res = self.serial.readline().strip()
		if "delay=" in res:
			return int(res.split('=')[1])

	def set_animation_delay(self, delay):
		assert(0<delay<256)
		self.serial.write('#'+chr(delay))
		res = self.serial.readline().strip()
		if "delay=" in res:
			return int(res.split('=')[1])

	def get_analogs(self):
		self.serial.write('@')
		res = json.loads(self.serial.readline())

	def set_power_on(self):
		self.serial.write('-')
		return "powerup" in self.serial.readline()

	def set_power_off(self):
		self.serial.write('_')
		return "powerdown" in self.serial.readline()

if __name__ ==  "__main__":
	import os
	import signal

	a = Ambianceduino()
	print("Got Ambianceduino")

	def signal_handler(signum, stack):
		if signum == signal.SIGUSR1:
			a.set_power_on()
		elif signum == signal.SIGUSR2:
			a.set_power_off()

	signal.signal(signal.SIGUSR1, signal_handler)
	signal.signal(signal.SIGUSR2, signal_handler)

	while True:
		sleep(0.3)
