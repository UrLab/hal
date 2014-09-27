HALFS_ROOT = "/dev/hal"
SENSORS_GRAPHITE = "localhost"

try:
    from local_config import *
except ImportError:
    pass
