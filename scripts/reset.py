#!/usr/bin/python

from math import sin, pi
from glob import glob
from config import HALFS_ROOT

def sinusoid(n_frames, min, max):
    assert min <= max
    assert 0 <= n_frames < 256
    assert 0 <= min < 256
    assert 0 <= max < 256

    a = (max-min)/2
    m = min+a
    return ''.join([chr(int(m + a*sin(2*i*pi/n_frames))) for i in range(n_frames)])

if __name__ == "__main__":
    for dest in glob(HALFS_ROOT+"/animations/*"):
        open(dest+"/frames", "w").write(sinusoid(120, 0, 255))
        open(dest+"/fps", 'w').write("25")
        open(dest+"/play", 'w').write("1")
        open(dest+"/loop", 'w').write("1")
