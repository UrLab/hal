from subprocess import Popen, PIPE

def sh(command):
	return Popen(command.split(' '), stdout=PIPE).communicate()[0].strip()

FIRMWARE_VERSION = sh("git rev-parse HEAD")
