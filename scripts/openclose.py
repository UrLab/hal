import hal
from time import sleep
from logging import getLogger

log = getLogger(__name__)


def open_hs():
    log.info("OPEN the hackerspace")
    for anim in ("red", "green", "blue"):
        prefix = "animations/" + anim
        hal.set(prefix + "/loop", True)
        hal.set(prefix + "/play", True)
        hal.set(prefix + "/fps", 50)
        hal.write(prefix + "/frames", hal.sinusoid(250, 0, 200))
        sleep(0.0001)

    hal.set("switchs/power", True)
    hal.set("switchs/leds_stairs", True)


def close_hs():
    log.info("CLOSE the hackerspace")

    # Shotdown all leds
    for anim in ("red", "green", "blue", "heater", "door_green"):
        prefix = "animations/" + anim
        hal.set(prefix + "/play", False)

    # Fix green light and stairs leds for 1 minute
    hal.write("animations/green/frames", chr(255))
    hal.set("animations/green/play", True)
    hal.set("switchs/leds_stairs", True)
    sleep(60)

    # Shut down everything
    hal.set("animations/green/play", False)
    hal.set("switchs/leds_stairs", False)
    hal.set("switchs/power", False)

if __name__ == "__main__":
    if hal.get("triggers/knife_switch"):
        open_hs()

    for trigger_name, state in hal.events():
        if trigger_name != 'knife_switch':
            continue

        if state is True:
            open_hs()
        else:
            close_hs()
