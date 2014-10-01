from config import get_hal
import internet
from time import sleep

hal = get_hal()
logger = hal.getLogger(__name__)

def illuminate_stairs(dt=60):
    logger.info("Illuminate stairs")
    for anim in ("red", "heater", "blue", "door_green"):
        hal.stop(anim)

    hal.upload("green", chr(0xff))
    hal.play("green")
    hal.on("leds_stairs")
    hal.on("power")
    sleep(dt)

    if not hal.trig('knife_switch'):
        hal.stop("green")
        hal.off("leds_stairs")
        hal.off("power")
        logger.info("Put off light in stairs")

def main():
    for trig_name, trig_active in hal.events():
        if trig_name == 'door_stairs' and trig_active and not hal.trig('knife_switch'):
            internet.lechbot_event('door_stairs')
            illuminate_stairs()



if __name__ == "__main__":
    main()
