HALFS_ROOT = "/dev/hal"
RMQ_HOST = "amqp://guest:guest@localhost"
SENSORS_GRAPHITE = "localhost"

try:
    from local_config import *
except ImportError:
    pass
