from config import get_hal
import internet
from light_stairs import illuminate_stairs
from time import sleep

hal = get_hal()
log = hal.getLogger(__name__)

def open_hs(called_on_trigger):
    log.info("OPEN the hackerspace")
    for anim in ("red", "green", "blue", "kitchen"):
        hal.upload(anim, hal.sinusoid(250, 0, 200))
        hal.loop(anim)
        hal.play(anim)
        hal.fps(anim, 50)
        sleep(0.0001)

    hal.upload("bell_eyes", [255])
    hal.one_shot("door_green")
    hal.play("heater")
    hal.on("power")
    hal.on("leds_stairs")
    hal.on("ampli")
    if called_on_trigger:
        internet.lechbot_event('hs_open')
        internet.spaceapi_open()

def close_hs(called_on_trigger):
    log.info("CLOSE the hackerspace")
    if called_on_trigger:
        internet.lechbot_event('hs_close')

    hal.off("ampli")
    # Shotdown all leds
    for anim in ("red", "green", "blue", "heater", "door_green", "kitchen"):
        hal.stop(anim)
        sleep(0.0001)
    
    if called_on_trigger:
        illuminate_stairs()
        internet.spaceapi_close()
    hal.upload("bell_eyes", [0])


def main():
    if hal.trig("knife_switch"):
        open_hs(called_on_trigger=False)
    else:
        close_hs(called_on_trigger=False)

    for trigger_name, state in hal.events():
        if trigger_name != 'knife_switch':
            continue

        if state is True:
            open_hs(called_on_trigger=True)
        else:
            close_hs(called_on_trigger=True)


if __name__ == "__main__":
    main()
