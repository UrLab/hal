import hal
from light_stairs import illuminate_stairs
from time import sleep

log = hal.getLogger(__name__)

def open_hs():
    log.info("OPEN the hackerspace")
    for anim in ("red", "green", "blue", "kitchen"):
        hal.upload(anim, hal.sinusoid(250, 0, 200))
        hal.loop(anim)
        hal.play(anim)
        hal.fps(anim, 50)
        sleep(0.0001)

    hal.one_shot("door_green")
    hal.play("heater")
    hal.on("power")
    hal.on("leds_stairs")
    hal.on("ampli")


def close_hs():
    log.info("CLOSE the hackerspace")

    hal.off("ampli")
    # Shotdown all leds
    for anim in ("red", "green", "blue", "heater", "door_green", "kitchen"):
        hal.stop(anim)
        sleep(0.0001)
    
    illuminate_stairs()


def main():
    if hal.trig("knife_switch"):
        open_hs()

    for trigger_name, state in hal.events():
        if trigger_name != 'knife_switch':
            continue

        if state is True:
            open_hs()
        else:
            close_hs()


if __name__ == "__main__":
    main()
