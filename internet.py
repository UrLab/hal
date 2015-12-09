import asyncio
from datetime import datetime
from crossbarconnect import Client as CrossbarClient
from config import WAMP_URL

from logging import getLogger
logger = getLogger()

TIMEFMT = '%Y-%m-%d %H:%M:%S'

client = CrossbarClient(WAMP_URL)


@asyncio.coroutine
def publish_hal_event(key, text):
    now = datetime.now().strftime(TIMEFMT)
    yield from client.publish('hal.eventstream', key=key, text=text, time=now)
