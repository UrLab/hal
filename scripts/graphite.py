from graphite_feeder import GraphiteFeeder
import hal
from config import SENSORS_GRAPHITE

logger = hal.getLogger(__name__)

server = GraphiteFeeder(SENSORS_GRAPHITE, prefix="hal", delay=60)

@server.metric(multiple=True)
def sensors():
    logger.info("Retrieve sensors...")
    return hal.sensors()

if __name__ == '__main__':
    server.feed()
