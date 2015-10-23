HALFS_ROOT = "/hal"

RMQ_HOST = "localhost"
RMQ_USER = "guest"
RMQ_PASSWORD = "guest"

LECHBOT_EVENTS_QUEUE = "test.hal.events"
LECHBOT_NOTIFS_QUEUE = "test.hal.notifs"

SENSORS_GRAPHITE = "localhost"

STATUS_CHANGE_URL = "http://localhost/"
STATUS_GET_URL = "http://localhost/"

PAMELA_URL = "http://localhost/mac.json"

SENTRY_URL = ""

try:
    from local_config import *
except ImportError:
    pass
