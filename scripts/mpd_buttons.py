from config import get_hal

hal = get_hal()
logger = hal.getLogger(__name__)

import os

def main():
    for ev, ev_on in hal.events():
        if not ev_on:
            continue

        if ev == "button_down":
            logger.info("button_down pressed")
            os.system("mpc volume -5")


        if ev == "bouton_play":
            logger.info("button_play pressed")
            os.system("mpc toggle")


        if ev == "button_up":
            logger.info("button_up pressed")
            os.system("mpc volume +5")



if __name__ == "__main__":
    main()


