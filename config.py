HALFS_ROOT = "/hal"

RMQ_HOST = "localhost"
RMQ_USER = "guest"
RMQ_PASSWORD = "guest"

LECHBOT_EVENTS_QUEUE = "test.hal.events"
LECHBOT_NOTIFS_QUEUE = "test.hal.notifs"

WAMP_REALM = "urlab"
WAMP_HOST = "ws://localhost:8080/ws"

SENSORS_GRAPHITE = "localhost"

STATUS_CHANGE_URL = "http://localhost/"
STATUS_GET_URL = "http://localhost/"

INCUBATOR_STATUS_CHANGE_URL = "http://localhost/"
INCUBATOR_SECRET = "thisisnosecret"

PAMELA_URL = "http://incubator.urlab.be/space/pamela"

SENTRY_URL = ""

INFLUX_URL = "http://hal:hal@localhost:8026/write?db=hal"

try:
    from local_config import *
except ImportError:
    pass
