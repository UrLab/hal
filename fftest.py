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

AUDIO_SAMPLE_RATE = 44100
LEDS_FPS = 50

SAMPLES_PER_FRAME = AUDIO_SAMPLE_RATE/LEDS_FPS
FRAMES_PER_PACK = 5

a = Ambianceduino()
a.on()
a.delay(1000/LEDS_FPS)
print "Got Ambianceduino"

latency = 1000/(LEDS_FPS/FRAMES_PER_PACK)
print "FPS:", LEDS_FPS, "Latency:", latency, "ms"

try:
    with open("/tmp/mpd.fifo", "rb") as sound:
        samples = numpy.zeros(SAMPLES_PER_FRAME)
        pack_r = [0 for i in range(FRAMES_PER_PACK)]
        pack_b = [0 for i in range(FRAMES_PER_PACK)]
        while True:
            for j in range(FRAMES_PER_PACK):
                for i in range(SAMPLES_PER_FRAME):
                    try:
                        msb = ord(sound.read(1))
                        ord(sound.read(1))
                    except:
                        exit()
                    samples[i] = (msb&0x7f) - (msb&0x80) #2's complement
                res = numpy.fft.fft(samples)
                n = len(res)
                t = n/2
                pack_r[j] = int(numpy.absolute(sum(res[:t])/n))/2
                pack_b[j] = int(numpy.absolute(sum(res[t:])/n))/2
            print "---"
            a.upload_anim('R', pack_r)
            a.upload_anim('B', pack_b)
            print("%3d %3d"%(pack_r[-1], pack_b[-1]))
except:
    a.off()