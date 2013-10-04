from serial import Serial
from glob import glob
from time import sleep
import json

class AmbianceduinoNotFound(Exception):
	pass

class Ambianceduino:
	DEV_PATTERNS = ["/dev/ttyACM*", "/dev/ttyUSB*"]

	def __init__(self, boot_time=3):
		possible_devices = [f for pattern in self.DEV_PATTERNS for f in glob(pattern)]
		for device in possible_devices:
			self.serial = Serial(device, 115200, timeout=1)
			sleep(boot_time) #Wait arduino boot
			self.serial.write('?')
			got = self.serial.readline()
			#Magic string
			if "?jesuisuncanapequichante" in got:
				break
			else:
				print(got)
				self.serial.close()
				self.serial = None
		if not self.serial:
			raise AmbianceduinoNotFound(str(possible_devices))

	def magic(self):
		self.serial.write('?')
		res = self.serial.readline().strip()
		if res[0] == '?':
			return res[1:]

	def delay(self, delay=None):
		query = '#'
		if delay in range(0, 256):
			query += chr(delay)
		self.serial.write(query)
		res = self.serial.readline().strip()
		if res[0] == '#':
			return int(res[1:])
		else: print res

	def analogs(self):
		self.serial.write('@')
		res = self.serial.readline().strip()
		if res[0] == '@':
			return json.loads(res[1:])

	def upload_anim(self, curve):
		dots = []
		for dot in curve:
			if 0 <= dot < 256: dots.append(chr(dot))
		self.serial.write('R' + chr(len(dots)) + ''.join(dots))
		res = self.serial.readline().strip()
		if res == "R":
			return True
		elif res == "!R":
			return False

	def on(self):
		self.serial.write('-')
		return self.serial.readline().strip() == '-'

	def off(self):
		self.serial.write('_')
		return self.serial.readline().strip() == '_'

if __name__ ==  "__main__":
	import os
	import signal

	a = Ambianceduino()
	print "magic:", a.magic()
	print "analogs:", a.analogs()
	print "upload:", a.upload_anim([3,3,2,5,4,7,9,2,1,0,12,3])
	print "delay 30:", a.delay(30)
	print "power on:", a.on()
	sleep(5)
	print "power off:", a.off()