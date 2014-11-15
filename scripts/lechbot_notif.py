#!/usr/bin/python

from config import get_hal
import internet
from time import sleep
from bell import funkytown

{
    "trash" : Partition(
        Note(220), Note(262), Note(330), Note(440), Note(494), Note(262), Note(330),
        Note(494), Note(523), Note(330), Note(262), Note(523), Note(370), Note(294),
        Note(220), Note(370), Note(349), Note(262), Note(220), Note(262, 2),
        Note(392), Note(392, 0.5), Note(330, 0.5), Note(262), Note(247), Note(262),
        Note(262, 2)
    )
}

hal = get_hal()
logger = hal.getLogger(__name__)

def main():
    for notification in internet.lechbot_notifications():
        if notification['name'] == "trash":
            hal.upload("buzzer", Stairways_to_heaven.to_frames())
            hal.play("buzzer")
            sleep(7)
            hal.upload("buzzer", funkytown.to_frames())
