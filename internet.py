import asyncio
import aioamqp
import json
import traceback
from datetime import datetime
from config import RMQ_HOST, RMQ_USER, RMQ_PASSWORD
from config import LECHBOT_EVENTS_QUEUE, LECHBOT_NOTIFS_QUEUE

TIMEFMT = '%Y-%m-%d %H:%M:%S'


@asyncio.coroutine
def rmq_error_callback(exc):
    print("Error when connecting to RabbitMQ:", exc)


@asyncio.coroutine
def lechbot_notif_consume(coroutine):
    try:
        transport, protocol = yield from aioamqp.connect(
            host=RMQ_HOST, login=RMQ_USER, password=RMQ_PASSWORD,
            on_error=rmq_error_callback)
        channel = yield from protocol.channel()
        queue = yield from channel.queue_declare(LECHBOT_NOTIFS_QUEUE)

        @asyncio.coroutine
        def consume(body, envelope, properties):
            yield from channel.basic_client_ack(envelope.delivery_tag)
            try:
                msg = json.loads(body.decode())
                now = datetime.now()
                msgtime = datetime.strptime(
                    msg.get('time', now.strftime(TIMEFMT)), TIMEFMT)
                if (now - msgtime).total_seconds() < 120:
                    yield from coroutine(msg['name'])
            except:
                traceback.print_exc()

        yield from channel.basic_consume(LECHBOT_NOTIFS_QUEUE, callback=consume)
    except aioamqp.AmqpClosedConnection:
        print("closed connections")
        return


@asyncio.coroutine
def lechbot_event(event_name):
    try:
        transport, protocol = yield from aioamqp.connect(
            host=RMQ_HOST, login=RMQ_USER, password=RMQ_PASSWORD,
            on_error=rmq_error_callback)
        channel = yield from protocol.channel()
        queue = yield from channel.queue_declare(LECHBOT_EVENTS_QUEUE)
        msg = {'time': datetime.now().strftime(TIMEFMT), 'name': event_name}
        yield from channel.publish(json.dumps(msg), '', LECHBOT_EVENTS_QUEUE)
    except aioamqp.AmqpClosedConnection:
        print("closed connections")
        return