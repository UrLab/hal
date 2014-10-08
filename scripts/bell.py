from config import get_hal
import internet
from halpy.generators import Partition, Note, Silence
from time import sleep

hal = get_hal()
logger = hal.getLogger(__name__)

funkytown = Partition(
    Note(494), Note(494), Note(440), Note(494, 2), Note(370, 2), Note(370), 
    Note(494), Note(659), Note(622), Note(494, 2), Silence(3)
)

BLINK = ['green', 'kitchen', 'blue']

def main():
    hal.stop('buzzer')
    hal.fps('buzzer', 17)
    hal.upload('buzzer', 2 * funkytown.to_frames())
    hal.one_shot('buzzer')
    hal.fps('bell_eyes', 60)

    for ev, ev_on in hal.events():
        if not ev_on or ev != 'bell':
            continue

        logger.info("Spotted someone at the door")
        hal.upload('bell_eyes', hal.sinusoid(n_frames=20, val_max=255))
        hal.play('buzzer')
        fps = {name: hal.fps(name) for name in BLINK}
        for name in BLINK:
            hal.fps(name, 450)
        internet.lechbot_event('bell')
        internet.events.send("bell", ["bell"])
        sleep(5)

        for name in BLINK:
            hal.fps(name, fps[name])
        hal.upload('bell_eyes', [255])

if __name__ == "__main__":
    main()
