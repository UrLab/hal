from graphite_feeder import GraphiteFeeder
from config import SENSORS_GRAPHITE, get_hal

from socket import gaierror
import time

hal = get_hal()
logger = hal.getLogger(__name__)
server = GraphiteFeeder(SENSORS_GRAPHITE, prefix="hal", delay=20)


@server.metric(multiple=True)
def sensors():
    logger.info("Retrieve sensors...")
    return hal.sensors()


def main():
    while True:
        try:
            server.feed()
        except gaierror:
            time.sleep(60)


if __name__ == '__main__':
    main()
