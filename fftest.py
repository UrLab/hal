import numpy as np
from struct import unpack

FPS = 25

bass_min, bass_max = 30, 240
treble_min, treble_max = 1000, 8000
n_samples = 44100


N = n_samples/2
samples_per_frame = n_samples/FPS

signal = np.zeros(n_samples*2)
fifo = open("/tmp/mpd.fifo")
samples = fifo.read(n_samples*2)
for i in range(n_samples):
    signal[i] = float(unpack('h', samples[2*i:2*i+2])[0])/2**15

begin, end = 0, n_samples
while True:
    fft = np.fft.fft(signal[begin:end])/N
    fft_real = abs(fft)[:N] #Only positives
    s = float(sum(fft_real))
    bass = int(255*sum(fft_real[bass_min:bass_max])/s)
    treble = int(255*sum(fft_real[treble_min:treble_max])/s)
    #print begin, end, "BASS:", bass, "TREBLE:", treble
    print "BASS: %3d   TREBLE: %3d" % (bass, treble)
    open("/dev/hal/animations/red/frames", "w").write(chr(bass))
    open("/dev/hal/animations/blue/frames", "w").write(chr(treble))

    samples = fifo.read(samples_per_frame*2)
    for i in range(samples_per_frame):
        signal[begin+i] = float(unpack('h', samples[2*i:2*i+2])[0])/2**15
        signal[end+i]

    begin += samples_per_frame
    end += samples_per_frame
    if end >= 2*n_samples:
        begin = 0
        end = n_samples
