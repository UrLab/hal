HALFS_ROOT = "/dev/hal"

RMQ_HOST = "amqp://guest:guest@localhost"
LECHBOT_EVENTS_QUEUE = "test.hal.events"
LECHBOT_NOTIFS_QUEUE = "test.hal.notifs"

SENSORS_GRAPHITE = "localhost"

STATUS_CHANGE_URL = "http://localhost/"
STATUS_GET_URL = "http://localhost/"

try:
    from local_config import *
except ImportError:
    pass

from halpy.hal import HAL

def get_hal():
    return HAL(HALFS_ROOT)
