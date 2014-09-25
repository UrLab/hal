from graphite_feeder import GraphiteFeeder
import hal

server = GraphiteFeeder("rainbowdash.lan", prefix="hal", delay=60)


@server.metric(multiple=True)
def sensors():
    return hal.sensors()

if __name__ == '__main__':
    server.feed()
