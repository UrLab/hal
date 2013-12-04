"""
Ensure the following lines are in /etc/mpd.conf:

audio_output {
    type    "fifo"
    name    "Visualizer"
    path    "/tmp/mpd.fifo"
    format  "44100:16:1"
}

"""

import numpy
from ambianceduino import Ambianceduino
import traceback
from math import log

AUDIO_SAMPLE_RATE = 44100
LEDS_FPS = 12
FRAMES_PER_PACK = 1
SAMPLES_PER_FRAME = AUDIO_SAMPLE_RATE/LEDS_FPS
SAMPLES_PER_PACK = SAMPLES_PER_FRAME*FRAMES_PER_PACK

SAMPLES_PER_FRAME = AUDIO_SAMPLE_RATE/LEDS_FPS
FRAMES_PER_PACK = 5

a = Ambianceduino()
a.run()
a.on()
a.delay(1000/LEDS_FPS)
print "Got Ambianceduino"

latency = 1000/(LEDS_FPS/FRAMES_PER_PACK)
print "FPS:", LEDS_FPS, "Latency:", latency, "ms"

try:
    N = 25
    pack_min, pack_max = 255, 0
    with open("/tmp/mpd.fifo", "rb") as sound:
        samples = numpy.zeros(SAMPLES_PER_FRAME)
        pack_r = [0 for i in range(FRAMES_PER_PACK)]
        pack_b = [0 for i in range(FRAMES_PER_PACK)]
        while True:
            for j in range(FRAMES_PER_PACK):
                for i in range(SAMPLES_PER_FRAME):
                    samples.append(getMsbFromAudio(sound)**2)
                #En = sqrt(sum(samples)/len(samples))
                Ek = sqrt(sum(samples[-N:])/N)
                #r = Ek/(En+1)
                pack[j] = 2*int(Ek) #min(int(En**r), 0xff)
            a.upload_anim('B', pack)
            if pack[-1] < pack_min:
                pack_min = pack[-1]
            elif pack[-1] > pack_max:
                pack_max = pack[-1]
            print pack_min, pack[-1], pack_max
except:
    traceback.print_exc()
    a.off()
    a.stop()
