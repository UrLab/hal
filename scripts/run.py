from time import sleep
from imp import load_source
from multiprocessing import Process
from config import get_hal
from setproctitle import setproctitle

logger = get_hal().getLogger()

TO_RUN = [
    "graphite",
    "openclose", 
    "light_stairs", 
    "bell", 
    "heatled", 
    "redled",
    "door_flash"
]

def wrap_main(name, main):
    def new_main():
        setproctitle("HAL script: " + name)
        logger.info("Starting " + name)
        while True:
            try:
                main()
            except:
                logger.exception("Restart " + name)
            sleep(60)
    return new_main

processes = []
try:
    for name in TO_RUN:
        mod = load_source(name, name+'.py')
        if hasattr(mod, 'main'):
            main = wrap_main(name, mod.main)
            processes.append(Process(target=main, name=name))
            logger.info("New process " + name)
            processes[-1].start()
            sleep(1)

    map(Process.join, processes)
except:
    map(Process.terminate, processes)
