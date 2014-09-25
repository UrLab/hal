import hal
from time import sleep

def open_hs():
    print "OPEN the hackerspace"
    for anim in ("red", "green", "blue"):
        prefix = "/animations/" + anim
        hal.set(prefix + "/loop", True)
        hal.set(prefix + "/play", True)
        hal.set(prefix + "/fps", 40)
        hal.write(prefix + "/frames", hal.sinusoid(250, 0, 200))
        sleep(0.0001)

    hal.set("/switchs/power", True)
    hal.set("/switchs/leds_stairs", True)

def close_hs():
    print "CLOSE the hackerspace"
    for anim in ("red", "green", "blue", "heater", "door_green"):
        prefix = "/animations/" + anim
        hal.set(prefix + "/play", False)
    hal.set("/switchs/leds_stairs", True)
    sleep(60)
    hal.set("/switchs/leds_stairs", False)
    hal.set("/switchs/power", False)


for trigger_name, state in hal.events():
    print trigger_name, state
    if trigger_name != 'knife_switch':
        continue

    if state is True:
        open_hs()
    else:
        close_hs()
    
