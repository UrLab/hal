import hal
from time import sleep

log = hal.getLogger(__name__)

def open_hs():
    log.info("OPEN the hackerspace")
    for anim in ("red", "green", "blue"):
        hal.upload(anim, hal.sinusoid(250, 0, 200))
        hal.loop(anim)
        hal.play(anim)
        hal.fps(anim, 50)
        sleep(0.0001)

    hal.on("power")
    hal.on("leds_stairs")


def close_hs():
    log.info("CLOSE the hackerspace")

    # Shotdown all leds
    for anim in ("red", "green", "blue", "heater", "door_green"):
        hal.stop(anim)

    # Fix green light and stairs leds for 1 minute
    hal.upload("green", chr(255))
    hal.play("green")
    hal.on("leds_stairs")
    sleep(60)

    # Shut down everything
    hal.stop("green", False)
    hal.off("leds_stairs", False)
    hal.off("power", False)


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
