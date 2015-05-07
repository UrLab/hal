from config import get_hal
from time import time
import internet
import signal

hal = get_hal()
logger = hal.getLogger(__name__)

BERNARD = ("green", "roof_g", "roof_b", "roof_r", "belgatop")


def stop_illuminate_stairs(*args, **kwargs):
    if not hal.trig('knife_switch'):
        for anim in BERNARD:
            hal.stop(anim)
        hal.off("leds_stairs")
        hal.off("power")
        hal.off("knife_r")
        hal.off("knife_g")
        hal.off("knife_b")
        logger.info("Put off light in stairs")


def illuminate_stairs(dt=90):
    logger.info("Illuminate stairs")
    for anim in ("red", "heater", "blue", "door_green"):
        hal.stop(anim)

    for anim in BERNARD:
        hal.upload(anim, chr(0xff))
        hal.play(anim)

    hal.on("leds_stairs")
    hal.on("power")

    signal.signal(signal.SIGALRM, stop_illuminate_stairs)
    signal.alarm(dt)


last_trig = 0


def main():
    global last_trig
    for trig_name, trig_active in hal.events():
        knife_on = hal.trig('knife_switch')
        door_open = trig_name == 'door_stairs' and trig_active
        if door_open and not knife_on and time() - last_trig > 60:
            last_trig = time()
            illuminate_stairs()
            hal.on("knife_b")
            hal.off("knife_g")
            hal.off("knife_r")
            internet.lechbot_event('door_stairs')
            internet.events.send('door_stairs', ["door_stairs"])


if __name__ == "__main__":
    main()
