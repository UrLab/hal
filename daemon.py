from ambianceduino import Ambianceduino
from urllib import urlopen
from datetime import datetime
import json
import puka
from math import log, sqrt
from time import sleep
from threading import Thread
from logbook import Logger, FileHandler

from config import AMQ_SERVER, METEO_QUEUE, METEO_LEN, EVENTS_QUEUE

def getMsbFromAudio(audio_source):
    msb = ord(audio_source.read(1))
    audio_source.read(1)
    return (msb&0x7f) - (msb&0x80) #2's complement

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
        self.logger = Logger('Daemon')
        super(AmbianceDaemon, self).__init__(*args, **kwargs)
        self.current_delay = None
        self.powered = None
        self.meteo = []
        self.__init_puka_client()
    
    def __spacestatus(self):
        try:
            statuspage = urlopen('http://api.urlab.be/spaceapi/status')
            payload = json.loads(statuspage.read())
            if 'state' in payload:
                if payload['state'] == 'open' and self.powered != True:
                    self.on()
                elif payload['state'] == 'closed' and self.powered != False:
                    self.off()
        except Exception as err:
            self.logger.error("%s: %s"%(err.__class__.__name__, err.message))
    
    def __peoplecount(self):
        try:
            pamelapage = urlopen('http://pamela.urlab.be/mac.json')
            payload = json.loads(pamelapage.read())
            people = len(payload['color']) + len(payload['grey'])
            d = int(25/log(people+2))
            if d != self.current_delay:
                self.delay(d)
        except Exception as err:
            self.logger.error("%s: %s"%(err.__class__.__name__, err.message))
    
    def __init_puka_client(self):
        try:
            self.puka_client = puka.Client(AMQ_SERVER)
            promise = self.puka_client.connect()
            self.puka_client.wait(promise)
            self.logger.info("Connected AMQ to %s"%(AMQ_SERVER))

            promise = self.puka_client.queue_declare(queue=METEO_QUEUE)
            self.puka_client.wait(promise)
            self.logger.info("Got queue %s"%(METEO_QUEUE))

            promise = self.puka_client.queue_declare(queue=EVENTS_QUEUE)
            self.puka_client.wait(promise)
            self.logger.info("Got queue %s"%(EVENTS_QUEUE))
        except:
            self.logger.error("Unable to connect to %s#%s"%(AMQ_SERVER, METEO_QUEUE))
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
            self.logger.info('Sent [%s] << %s'%(queue, msg[:50]))
            
    def __http_requests(self):
        i = 0
        while self.running:
            i = (i + 1) % 20
            self.analogs()
            if i == 0:
                self.__spacestatus()
                self.__peoplecount()
            sleep(1)
    
    def stop(self):
        super(AmbianceDaemon, self).stop()
        self.requestsThread.join()
    
    def run(self):
        super(AmbianceDaemon, self).run()
        self.requestsThread = Thread(target=self.__http_requests)
        self.requestsThread.start()
    
    def default_handler(self, *args):
        self.logger.info(' '.join(map(str, args)))

    def when_on(self):
        self.powered = True
        self.logger.info('Powered on')

    def when_off(self):
        self.powered = False
        self.logger.info('Powered off')

    def when_delay(self, delay):
        self.current_delay = delay
        self.logger.info('Delay set to %d'%(delay))

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
    
    def when_analogs(self, analogs):
        self.meteo.append(analogs)
        if len(self.meteo) == METEO_LEN:
            self.__send_message(METEO_QUEUE ,{
                'time': str(datetime.now()),
                'light' : {
                    'inside': avg_for_key(self.meteo, 'light_in')/10.23,
                    'outside': avg_for_key(self.meteo, 'light_out')/10.23
                },
                'temperature' : {
                    'radiator': convertThermistor(avg_for_key(self.meteo, 'temp_radia'), 4700, 3950),
                    'ambiant': convertThermistor(avg_for_key(self.meteo, 'temp_amb'), 2200, 3950)
                }
            })
            self.meteo = []

    def mainloop(self):
        SAMPLES_PER_FRAME = 25
        AUDIO_SAMPLE_RATE = 44100
        FRAMES_PER_SECOND = 14
        SAMPLES_TO_DROP = (AUDIO_SAMPLE_RATE/FRAMES_PER_SECOND)-SAMPLES_PER_FRAME
        with open("/tmp/mpd.fifo", "rb") as sound:
            while True:
                sound.read(SAMPLES_TO_DROP*2)
                s = 0
                for i in range(SAMPLES_PER_FRAME):
                    s += getMsbFromAudio(sound)**2
                x = sqrt(s/SAMPLES_PER_FRAME)
                self.upload_anim('B', [int(1.0446300614125956**x)-1])
            

if __name__ == "__main__":

    log_handler = FileHandler('daemon.log')
    log_handler.push_application()
    logger = Logger('Main')
    logger.info('Starting')
    
    try:
        a = AmbianceDaemon(boot_time=10)
    except Exception as err:
        logger.error("%s: %s"%(err.__class__.__name__, err))
        exit(1)

    logger.info('Got Ambianceduino')



    try:
        a.run()
        a.mainloop()
    except KeyboardInterrupt:
        a.stop()
        logger.info('Stopping')
        exit()
    except Exception as err:
        logger.error("%s: %s"%(err.__class__.__name__, err.message))

