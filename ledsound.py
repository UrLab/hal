import struct
from math import sqrt

SAMPLE_RATE = 44100
FPS = 100
SPF = int(SAMPLE_RATE / FPS)

unpack_fmt = "@%dH" % SPF
print(unpack_fmt)

with open("/var/run/mpd/output.fifo", 'rb') as fifo:
    with open("/hal/animations/blue/frames", 'wb') as out:
        last_val = 0.0
        while True:
            samples = struct.unpack(unpack_fmt, fifo.read(2*SPF))
            s = 0
            for i in range(1, len(samples)):
                d = (samples[i] - samples[i-1]) / 65536.
                s += d*d
            val = s/SPF
            last_val = last_val*0.75 + val*0.25
            try:
                out.write(bytes([int(255*sqrt(last_val))]))
                out.flush()
            except Exception as err:
                print("WRITE ERROR", err)
                pass

