from ambianceduino import Ambianceduino
from urllib import urlopen
from datetime import datetime
import json
import puka
from math import log, sqrt
from time import sleep
from threading import Thread
from logbook import Logger, FileHandler
import traceback
import sys
import os
from contextlib import contextmanager

from config import *

# @contextmanager
# def nbopen(name, flag):
#     f = os.open(name, flag)
#     fo = os.fdopen(f)
#     yield fo
#     os.close(f)


def getMsbFromAudio(audio_source):
    r = audio_source.read(1)
    if not r:
        return 0
    msb = ord(r)
    audio_source.read(1)
    return (msb & 0x7f) - (msb & 0x80) # 2's complement


def avg_for_key(list_of_dictionaries, key):
    """
    [{'a': 1}, {'a': 5}], 'a' # => 3
    """
    return sum([x[key] for x in list_of_dictionaries]) / len(list_of_dictionaries)


def convertThermistor(analogValue, resistance, beta):
    vout = 5 * analogValue / 1023.0
    r = resistance * (5 - vout) / vout
    return (beta / (log(r / resistance) + beta / 298.15)) - 273.15


class AmbianceDaemon(Ambianceduino):

    def __init__(self, *args, **kwargs):
        self.logger = Logger('Daemon')
        super(AmbianceDaemon, self).__init__(*args, **kwargs)
        self.current_delay = None
        self.powered = None
        self.meteo = []
        self.__init_puka_client()
        self.anims_uploaded = 0

    def delay_red(self, milliseconds):
        self.delay('R', milliseconds)

    def delay_blue(self, milliseconds):
        self.delay('B', milliseconds)

    def __spacestatus(self):
        try:
            statuspage = urlopen(STATUS_URL)
            payload = json.loads(statuspage.read())
            if 'state' in payload:
                if payload['state'] == 'open' and not self.powered:
                    self.on()
                elif payload['state'] == 'closed' and self.powered:
                    self.off()
        except Exception as err:
            self.logger.error("%s: %s" % (err.__class__.__name__, err.message))

    def __peoplecount(self):
        try:
            pamelapage = urlopen('https://pamela.urlab.be/mac.json')
            payload = json.loads(pamelapage.read())
            people = len(payload['color']) + len(payload['grey'])
            d = int(25 / log(people + 2))
            if d != self.current_delay:
                self.delay_red(d)
        except Exception as err:
            self.logger.error("%s: %s" % (err.__class__.__name__, err.message))

    def __init_puka_client(self):
        try:
            self.puka_client = puka.Client(AMQ_SERVER)
            promise = self.puka_client.connect()
            self.puka_client.wait(promise)
            self.logger.info("Connected AMQ to %s" % (AMQ_SERVER))

            promise = self.puka_client.queue_declare(queue=METEO_QUEUE)
            self.puka_client.wait(promise)
            self.logger.info("Got queue %s" % (METEO_QUEUE))

            promise = self.puka_client.queue_declare(queue=EVENTS_QUEUE)
            self.puka_client.wait(promise)
            self.logger.info("Got queue %s" % (EVENTS_QUEUE))
        except:
            self.logger.error("Unable to connect to %s#%s" % (AMQ_SERVER, METEO_QUEUE))
            self.puka_client = None

    def __send_message(self, queue, json_dumpable):
        if self.puka_client:
            msg = json.dumps(json_dumpable)
            promise = self.puka_client.basic_publish(
                exchange='',
                routing_key=queue,
                body=msg
            )
            self.puka_client.wait(promise)
            self.logger.info('Sent [%s] << %s' % (queue, msg[:50]))

    def __http_requests(self):
        i = 0
        while self.running:
            self.analogs()
            if i == 0:
                self.__spacestatus()
                self.__peoplecount()
            i = (i + 1) % 20
            sleep(1)

    def stop(self):
        with open("/tmp/mpd.fifo", "w") as fifo:
            fifo.write(chr(0) * (80200 / 14))
        super(Ambianceduino, self).stop()

    def run(self):
        super(AmbianceDaemon, self).run()
        self.requestsThread = Thread(target=self.__http_requests, name="http-request")
        self.threads.append(self.requestsThread)
        self.requestsThread.start()
        self.mpdThread = Thread(target=self.__mpd_loop, name="mpd")
        self.threads.append(self.mpdThread)
        self.mpdThread.start()
        while True:
            for t in self.threads:
                if not t.is_alive():
                    self.logger.error("One of my children is dead, suicide.")
                    self.stop()
                    exit()
            sleep(0.5)

    def default_handler(self, *args):
        self.logger.info(' '.join(map(str, args)))

    def when_on(self):
        self.powered = True
        self.logger.info('Powered on')

    def when_off(self):
        self.powered = False
        self.logger.info('Powered off')

    def when_delay(self, output, delay):
        if output == 'R':
            self.current_delay = delay
            self.logger.info('Delay RED set to %d' % (delay))
        elif output == 'B':
            self.logger.info('Delay BLUE set to %d' % (delay))

    def when_bell(self):
        self.__send_message(EVENTS_QUEUE, {
            'trigger': 'bell', 'time': str(datetime.now())
        })

    def when_door(self):
        if not self.powered:
            self.__send_message(EVENTS_QUEUE, {
                'trigger': 'door', 'time': str(datetime.now())
            })

    def when_radiator(self):
        self.__send_message(EVENTS_QUEUE, {
            'trigger': 'radiator', 'time': str(datetime.now())
        })

    def when_anim(self, anim_name, anim_length):
        self.anims_uploaded += 1

    def when_analogs(self, analogs):
        self.meteo.append(analogs)
        if len(self.meteo) == METEO_LEN:
            self.__send_message(METEO_QUEUE, {
                'time': str(datetime.now()),
                'light': {
                    'inside': avg_for_key(self.meteo, 'light_in') / 10.23,
                    'outside': avg_for_key(self.meteo, 'light_out') / 10.23
                },
                'temperature': {
                    'radiator': convertThermistor(avg_for_key(self.meteo, 'temp_radia'), 4700, 3950),
                    'ambiant': convertThermistor(avg_for_key(self.meteo, 'temp_amb'), 2200, 3950)
                }
            })
            self.meteo = []

    def change_hs_status(self, status):
        try:
            urlopen('http://api.urlab.be/spaceapi/statuschange?status='+status)
        except:
            pass
        self.__send_message(EVENTS_QUEUE, {
            'trigger': 'hs_'+status, 'time': str(datetime.now())
        })

    #http://api.urlab.be/spaceapi/statuschange?status=(open|close)
    def when_hs_open(self):
        self.change_hs_status('open')
        
    def when_hs_close(self):
        self.change_hs_status('close')

    def __mpd_loop(self):
        SAMPLES_PER_FRAME = 25
        AUDIO_SAMPLE_RATE = 44100
        FRAMES_PER_SECOND = 14
        SAMPLES_TO_DROP = (AUDIO_SAMPLE_RATE / FRAMES_PER_SECOND) - SAMPLES_PER_FRAME

        self.delay('B', int(1000 / FRAMES_PER_SECOND))
        #with nbopen("/tmp/mpd.fifo", os.O_RDONLY | os.O_NONBLOCK) as sound:
        with open("/tmp/mpd.fifo", "rb") as sound:
            while self.running:
                if not self.powered:
                    sleep(1)
                    continue
                sound.read(SAMPLES_TO_DROP * 2)
                s = 0
                for i in range(SAMPLES_PER_FRAME):
                    s += getMsbFromAudio(sound) ** 2
                x = sqrt(s / SAMPLES_PER_FRAME)
                self.upload_anim('B', [int(1.0446300614125956 ** x) - 1])

if __name__ == "__main__":

    # log_handler = FileHandler('daemon.log')
    # log_handler.push_application()
    logger = Logger('Main')
    logger.info('Starting')

    try:
        a = AmbianceDaemon(logger=logger, boot_time=2)
        logger.info('Got Ambianceduino version ' + a.version)
        a.run()
    except KeyboardInterrupt:
        a.stop()
        logger.info('Stopping')
        exit()
    except Exception as err:

        logger.error("%s: %s" % (err.__class__.__name__, err.message))
        print traceback.format_exc()
        try:
            a.stop()
        except NameError:
            pass
