from config import get_hal
import internet
import signal

hal = get_hal()
logger = hal.getLogger(__name__)


def stop_illuminate_stairs(*args, **kwargs):
    if not hal.trig('knife_switch'):
        hal.stop("green")
        hal.off("leds_stairs")
        hal.off("power")
        logger.info("Put off light in stairs")


def illuminate_stairs(dt=90):
    logger.info("Illuminate stairs")
    for anim in ("red", "heater", "blue", "door_green"):
        hal.stop(anim)

    hal.upload("green", chr(0xff))
    hal.play("green")
    hal.on("leds_stairs")
    hal.on("power")

    signal.signal(signal.SIGALRM, stop_illuminate_stairs)
    signal.alarm(dt)


def main():
    for trig_name, trig_active in hal.events():
        if trig_name == 'door_stairs' and trig_active and not hal.trig('knife_switch'):
            illuminate_stairs()
            internet.lechbot_event('door_stairs')
            internet.events.send('door_stairs', ["door_stairs"])


if __name__ == "__main__":
    main()
