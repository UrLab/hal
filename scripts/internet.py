import puka
import json
from datetime import datetime
import requests

from config import RMQ_HOST, LECHBOT_EVENTS_QUEUE, STATUS_CHANGE_URL

def lechbot_event(name):
    """
    Emit given event name to LechBot's event queue. It will be translated to
    #urlab if necessary.
    See also: https://github.com/titouanc/lechbot/blob/master/plugins/HAL.rb
    """
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

def pamela():
    """
    Retrieve pamela data
    """
    return json.loads(requests.get("http://pamela.urlab.be/mac.json").content)

def spaceapi_open():
    return requests.get(STATUS_CHANGE_URL+"?status=open").status_code == 200

def spaceapi_close():
    return requests.get(STATUS_CHANGE_URL+"?status=close").status_code == 200
