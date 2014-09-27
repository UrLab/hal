HALFS_ROOT = "/dev/hal"

RMQ_HOST = "amqp://guest:guest@localhost"
LECHBOT_EVENTS_QUEUE = "test.hal.events"

SENSORS_GRAPHITE = "localhost"

STATUS_CHANGE_URL = "http://localhost/"

try:
    from local_config import *
except ImportError:
    pass
