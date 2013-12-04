from serial import Serial
from glob import glob
from time import sleep
from threading import Thread
from math import sqrt
import json

from version import FIRMWARE_VERSION

class AmbianceduinoFinder(object):
    class NotFound(Exception):
        pass
    
    class VersionMismatch(Exception):
        pass
    
    DEV_PATTERNS = ["/dev/ttyACM*", "/dev/ttyUSB*", "/dev/tty.usbmodem*"]

    def __try_device(self, device, boot_time, tries=10):
        self.serial = Serial(device, 115200, timeout=1)
        sleep(boot_time) #Wait arduino boot
        self.serial.write('?')
        done = False
        for i in range(tries):
            got = self.serial.readline().strip()
            #Magic string
            if got and got[0] == '?':
                if got[1:] == FIRMWARE_VERSION:
                    done = True
                    self.version = FIRMWARE_VERSION
                    break
                else:
                    raise self.VersionMismatch("Expected %s; got %s"%(FIRMWARE_VERSION, got[1:]))
        if not done:
            self.serial.close()
            self.serial = None

    def __init__(self, device_path=None, boot_time=5):
        if device_path:
            self.__try_device(device_path, boot_time)
        else:
            possible_devices = [f for pattern in self.DEV_PATTERNS for f in glob(pattern)]
            for device in possible_devices:
                self.__try_device(device, boot_time)
                if self.serial:
                    break
        if not self.serial:
            raise self.NotFound("Tried "+str(possible_devices+[device_path]))

    
class AmbianceduinoReader(AmbianceduinoFinder):
    def eval_line(self, line):
        if line[0] == '#':
            output = line[1]
            delay = int(line[2:])
            self.when_delay(output, delay)
        elif line[0] == 'T':
            active = bool(int(line[-1]))
            name = line[1:-1]
            self.when_trigger(name, active)
        elif line[0] == '@':
            analogs = json.loads(line[1:])
            self.when_analogs(analogs)
        elif line[0] == '!':
            self.when_error(line[1:])
        elif line[0] == '-':
            self.when_on()
        elif line[0] == '_':
            self.when_off()
        elif line[0] in ['R', 'B']:
            anim_length = int(line[1:])
            self.when_anim(line[0], anim_length)

    def read_loop(self):
        while self.running:
            line = self.serial.readline().strip()
            if len(line) > 0:
                self.eval_line(line)
            
    def run(self):
        self.running = True
        self.reader = Thread(target=self.read_loop)
        self.reader.start()

    def stop(self):
        self.running = False
        self.reader.join()

    def default_handler(self, *args):
        print ' '.join(map(str, args))

    def when_trigger(self, name, active):
        if active:
            if name == 'door':
                self.when_door()
            elif name == 'bell':
                self.when_bell()
            elif name == 'radiator':
                self.when_radiator()
            else:
                self.default_handler("Trigger %s %s"%(name, "rise" if active else "fall"))

    def when_delay(self, output, delay):
        self.default_handler("Delay for output", output, ":", delay)

    def when_analogs(self, analogs):
        self.default_handler("Analogs:", analogs)

    def when_anim(self, anim_name, anim_length):
        self.default_handler("Uploaded ", anim_name, " len:", anim_length)

    def when_bell(self):
        self.default_handler("Someone ring the bell !!!")

    def when_on(self):
        self.default_handler("Powered on")

    def when_off(self):
        self.default_handler("Powered off")

    def when_error(self, err_string):
        self.default_handler("Error:", err_string)

    def when_door(self):
        self.default_handler("The door is open !")
    
    def when_radiator(self):
        self.default_handler("The radiator is on !");

class AmbianceduinoWriter(AmbianceduinoFinder):
    ANIM_OUTPUTS = ['R', 'B']

    def __request(self, req_bytes):
        self.serial.write(req_bytes)

    def delay(self, output, delay=1):
        assert output in self.ANIM_OUTPUTS
        query = '#'+output
        if delay in range(1, 256):
            query += chr(delay)
        else:
            query += chr(0)
        self.__request(query)

    def analogs(self):
        self.__request('@')

    def upload_anim(self, output, curve):
        assert output in self.ANIM_OUTPUTS
        dots = []
        for dot in curve:
            if 0 <= dot < 256: dots.append(chr(dot))
        self.__request('U' + output + chr(len(dots)) + ''.join(dots))
    
    def reset_anim(self, output):
        assert output in self.ANIM_OUTPUTS
        self.__request('%'+output)
    
    def on(self):
        self.__request('-')

    def off(self):
        self.__request('_')


class Ambianceduino(AmbianceduinoReader, AmbianceduinoWriter):
    pass
