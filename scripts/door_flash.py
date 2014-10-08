import hal
import internet

logger = hal.getLogger(__name__)

def main():
    hal.upload('door_green', hal.sinusoid(n_frames=100)[:75])
    hal.fps('door_green', 75)
    hal.one_shot('door_green')
    logger.info('Door flash ready')
    for ev, ev_on in hal.events():
        if ev_on and ev == "passage":
            hal.play('door_green')
            logger.info('flash')
            internet.events.send("passage", ["passage"])

if __name__ == "__main__":
    main()
