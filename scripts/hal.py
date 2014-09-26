# HAL API
# Use the functions get() and set() to access driver values. These two functions
# do the right cast and you'll get a usable value instead of a string.

from math import sin, pi
from config import HALFS_ROOT
import socket
import glob
from os import path
import logging
from sys import stdout

def expand_path(filename):
    if filename.startswith(HALFS_ROOT):
        return filename
    return path.join(HALFS_ROOT, filename)


def read(filename):
    """ Returns a string with the content of the file given in parameter """
    return open(expand_path(filename), 'r').read().strip("\0").strip()


def write(filename, value):
    """ Casts value to str and writes it to the file given in parameter """
    open(expand_path(filename), 'w').write(str(value))


def get(filename):
    """ Returns float or int value depending on the type of the value in the file """
    str_value = read(filename)
    if str_value.find(".") == -1:  # value is an integer
        return int(str_value)
    else:  # value is a float
        return float(str_value)
    return 0


############ HIGH LEVEL API ############
def getLogger(*args, **kwargs):
    """Return a logger suitable for HAL scripts"""
    log = logging.getLogger(*args, **kwargs)
    log.setLevel(logging.DEBUG)
    ch = logging.StreamHandler(stdout)
    ch.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    ch.setFormatter(formatter)
    log.addHandler(ch)
    return log


def events():
    """
    Subsribe to hal events, and return an iterator (name, state)
    example: for trigger_name, trigger_active in events(): ...
    """
    events = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    events.connect(expand_path("events"))
    while True:
        line = events.recv(16)
        line = line.strip()
        trig_name, state = line.split(':')
        yield trig_name, (state == '1')


def waitFor(trigger, on_activation=True):
    """Return when trigger becomes (in)active"""
    for trig_name, trig_active in events():
        if trig_name == trigger and trig_active == on_activation:
            break


def sinusoid(n_frames=255, val_min=0, val_max=255):
    """
    Return one sinus period on n_frames varying between val_min and val_max.
    The returned string is suitable for upload()
    """
    if isinstance(val_min, float):
        val_min = 255*val_min
    if isinstance(val_max, float):
        val_max = 255*val_max
    assert val_min <= val_max
    for n in n_frames, val_min, val_max:
        assert 0 <= n < 256

    a = (val_max - val_min) / 2
    m = val_min + a
    return ''.join([chr(int(m + a * sin(2 * i * pi / n_frames))) for i in range(n_frames)])


def sensor(name):
    """Return name sensor"""
    return get("sensors/"+name)


def sensors():
    """Return all sensors values in a dict"""
    sensors = {}
    for sensor in glob.glob(path.join(HALFS_ROOT, "sensors", "*")):
        sensors[path.basename(sensor)] = get(sensor)

    return sensors


def trig(trigger):
    """Return true if trigger is active"""
    return get("triggers/" + trigger) == 1


def on(switch):
    """Put switch on"""
    write("switchs/" + switch, 1)


def off(switch):
    """Put switch off"""
    write("switchs/" + switch, 0)


def upload(anim, frames):
    """
    Upload frames to anim.
    Frames could be given in the following formats:
    * [float (0..1), ...]
    * [int (0..255), ...]
    * [chr, ...]
    * str
    """
    assert 0 < len(frames) < 256
    if isinstance(frames, list):
        if isinstance(frames[0], int):
            frames = ''.join(map(ord, frames))
        elif isinstance(frames[0], float):
            frames = ''.join(ord(int(255 * f)) for f in frames)
        elif isinstance(frames[0], str):
            frames = ''.join(frames)
    assert type(frames) in (str, unicode, bytes)
    write("animations/" + anim + "/frames", frames)


def fps(anim, fps=None):
    """Set anim speed in frames per second. If fps is none, only return its actual value"""
    if fps is None:
        return get("animations/" + anim + "/fps")
    else:
        assert 4 <= fps <= 1000
        write("animations/" + anim + "/fps", int(fps))


def play(anim):
    """Start playing anim"""
    write("animations/" + anim + "/play", 1)


def stop(anim):
    """Stop playing anim"""
    write("animations/" + anim + "/play", 0)


def loop(anim):
    """Put anim in loop mode"""
    write("animations/" + anim + "/loop", 1)


def one_shot(anim):
    """Put anim in one_shot mode"""
    write("animations/" + anim + "/loop", 0)
