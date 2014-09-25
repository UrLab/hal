from config import HALFS_ROOT
import socket

events = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
events.connect(HALFS_ROOT+"/events")

while True:
    line = events.recv(16)
    line = line.strip("\0").strip()
    trig_name, state = line.split(':')

    if trig_name == 'knife_switch':
        open(HALFS_ROOT+"/switchs/power", 'w').write(state)
