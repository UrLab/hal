from controller import *

a = AmbianceduinoReader()
print "Got Ambianceduino"

try:
	a.run()
	print "Read thread started"
	while True: sleep(0.1)
except:
	a.stop()
	print "Read thread stopped"

print "Good bye !"