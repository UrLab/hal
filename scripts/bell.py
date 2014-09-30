import hal
from time import sleep

FUNKYTOWN = [
    49, 49, 49, 0, 49, 49, 49, 0, 44, 44, 44, 0, 49, 49, 49, 49, 49, 49, 49, 0,
    37, 37, 37, 37, 37, 37, 37, 0, 37, 37, 37, 0, 49, 49, 49, 0, 66, 66, 66, 0,
    62, 62, 62, 0, 49, 49, 49, 49, 49, 49, 49, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0
]

logger = hal.getLogger(__name__)

def main():
    funkytown = [FUNKYTOWN[i]*(1+(i%2)) for i in range(len(FUNKYTOWN))]
    hal.stop('buzzer')
    hal.fps('buzzer', 17)
    hal.upload('buzzer', 2*funkytown)
    hal.one_shot('buzzer')
    while True:
        hal.waitFor('bell')
        logger.info("Spotted someone at the door")
        hal.upload('bell_eyes', hal.sinusoid(n_frames=20, val_max=255))
        hal.play('buzzer')
        sleep(2)
        hal.upload('bell_eyes', [255])

if __name__ == "__main__":
    main()
