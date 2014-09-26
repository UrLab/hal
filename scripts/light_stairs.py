import hal
from time import sleep

logger = hal.getLogger(__name__)

def illuminate_stairs():
    for anim in ("red", "heater", "blue", "door_green"):
        hal.stop(anim)

    hal.upload("green", [1.0])
    hal.play("green")
    hal.on("leds_stairs")
    hal.on("power")
    sleep(5)

    if not hal.trig('knife_switch'):
        hal.stop("green")
        hal.off("leds_stairs")
        hal.off("power")

def main():
    for trig_name, trig_active in hal.events():
        if trig_name == 'door_stairs' and trig_active and not hal.trig('knife_switch'):
            illuminate_stairs()



if __name__ == "__main__":
    main()
