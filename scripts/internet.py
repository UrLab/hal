import puka
import json
from datetime import datetime
import requests

import urllib
from socket import gaierror

from config import RMQ_HOST, LECHBOT_EVENTS_QUEUE, STATUS_CHANGE_URL


def lechbot_event(name):
    """
    Emit given event name to LechBot's event queue. It will be translated to
    #urlab if necessary.
    See also: https://github.com/titouanc/lechbot/blob/master/plugins/HAL.rb
    """
    try:
        client = puka.Client(RMQ_HOST)
        client.wait(client.connect())
        client.wait(client.queue_declare(queue=LECHBOT_EVENTS_QUEUE))
        now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        message = {
            'time': now,
            'trigger': name
        }
        client.wait(client.basic_publish(
            exchange='',
            routing_key=LECHBOT_EVENTS_QUEUE,
            body=json.dumps(message)
        ))
    except gaierror:
        pass


def pamela():
    """
    Retrieve pamela data
    """
    try:
        return json.loads(requests.get("http://pamela.urlab.be/mac.json").content)
    except urllib.request.URLError:
        return {
            "color": [],
            "hidden": 0,
            "grey": []
        }


def spaceapi_open():
    try:
        return requests.get(STATUS_CHANGE_URL + "?status=open").status_code == 200
    except urllib.request.URLError:
        return False


def spaceapi_close():
    try:
        return requests.get(STATUS_CHANGE_URL + "?status=close").status_code == 200
    except urllib.request.URLError:
        return False
