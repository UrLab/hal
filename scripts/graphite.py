from graphite_feeder import GraphiteFeeder
from config import SENSORS_GRAPHITE, get_hal

hal = get_hal()
logger = hal.getLogger(__name__)
server = GraphiteFeeder(SENSORS_GRAPHITE, prefix="hal", delay=60)

@server.metric(multiple=True)
def sensors():
    logger.info("Retrieve sensors...")
    return hal.sensors()

def main():
    server.feed()

if __name__ == '__main__':
    main()
