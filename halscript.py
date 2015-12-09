import asyncio
import aiohttp
from asyncio import subprocess
from math import log
import json
import raven
import os
from functools import wraps
from datetime import datetime
import random
import logging
from sys import stdout

from halpy import HAL
from halpy.generators import sinusoid, Partition, Note, Silence
from config import HALFS_ROOT, PAMELA_URL, INFLUX_URL, SENTRY_URL
from config import INCUBATOR_STATUS_CHANGE_URL, INCUBATOR_SECRET
from config import WAMP_REALM, WAMP_HOST
from autobahn.asyncio.wamp import ApplicationSession, ApplicationRunner

logging.basicConfig(
    stream=stdout, level=logging.INFO,
    format="%(asctime)s %(levelname)7s: %(message)s")
logger = logging.getLogger(__name__)

LIGHT_TIMEOUT = 60  # Seconds of light when passage or closing UrLab

funkytown = Partition(
    Note(494), Note(494), Note(440), Note(494, 2), Note(370, 2), Note(370),
    Note(494), Note(659), Note(622), Note(494, 2), Silence(3))

trashmusic = Partition(
    Note(220), Note(262), Note(330), Note(440), Note(494), Note(262), Note(330),
    Note(494), Note(523), Note(330), Note(262), Note(523), Note(370), Note(294),
    Note(220), Note(370), Note(349), Note(262), Note(220), Note(262, 2),
    Note(392), Note(392, 0.5), Note(330, 0.5), Note(262), Note(247), Note(262),
    Note(262, 2))

openmusic = Partition(
    Note(523), Note(659), Note(784), Note(1046, 2), Note(784), Note(1046, 2))

pokemusics = [
    (18, Partition(
        Note(440), Note(415), Note(440), Note(493), Note(523), Note(587), Note(659),
        Note(440), Note(493), Note(523), Note(587), Note(659), Note(698), Note(659),
        Note(698), Note(783), Note(880), Note(698), Note(659, 3))),

    (10, Partition(
        Note(440), Note(440), Note(440), Note(349, 0.75), Note(523, 0.25),
        Note(440), Note(349, 0.75), Note(523, 0.25), Note(440, 2)))
]

sentry = raven.Client(SENTRY_URL, release=raven.fetch_git_sha(os.path.dirname(__file__)))


def sentry_listen(fun):
    @wraps(fun)
    def wrapper(*args, **kwargs):
        try:
            return fun(*args, **kwargs)
        except Exception as e:
            sentry.captureException()
            raise
    return wrapper


hal = HAL(HALFS_ROOT)


class SafeBuzzer():
    """
    Buzzer and roof red ledstrip could not be on at the same time
    (hardware limitation).
    """
    def __enter__(self):
        self.initial_color = hal.rgbs.roof.color
        if self.initial_color[0]:
            hal.rgbs.roof.color = (0,) + self.initial_color[1:]
        return hal.animations.buzzer

    def __exit__(self, *args, **kwargs):
        if self.initial_color[0]:
            hal.rgbs.roof.color = self.initial_color


@asyncio.coroutine
@sentry_listen
def execute_command(*args):
    proc = yield from asyncio.create_subprocess_exec(
        *args,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout, _ = yield from proc.communicate()
    exit_code = yield from proc.wait()
    return exit_code


def set_urlab_open():
    for sw in ['power', 'ampli', 'leds_stairs']:
        hal.switchs[sw].on = True

    hal.rgbs.knife_leds.css = '#0f0'
    hal.rgbs.roof.css = '#fff'

    # Put fixed animations on
    for a in ['bell_eyes']:
        hal.animations[a].upload([1.0])
        hal.animations[a].playing = True
        hal.animations[a].looping = True

    # Start glowing ledstrips
    for a in ['belgatop', 'blue', 'green', 'heater', 'kitchen', 'red']:
        anim = hal.animations[a]
        anim.upload(sinusoid())
        anim.fps = 40
        anim.looping = True
        anim.playing = True

    heater_changed('heater', hal.triggers.heater.on)
    kitchen_changed('kitchen', hal.triggers.kitchen_move.on)


def set_urlab_closed(switchs_on=[], anims_fixed=[]):
    for sw in hal.switchs.values():
        sw.on = sw.name in switchs_on

    hal.rgbs.roof.css = '#f00'
    hal.rgbs.knife_leds.css = '#f00'

    for anim in hal.animations.values():
        if anim.name not in anims_fixed:
            anim.playing = False
        else:
            anim.upload([1.0])
            anim.looping = True
            anim.playing = True


@hal.on_trigger('heater')
@sentry_listen
def heater_changed(name, state):
    # light or shut down the heater ledstrip according to the valve
    hal.animations.heater.playing = state


@hal.on_trigger('kitchen_move')
@sentry_listen
def kitchen_changed(name, state):
    # light or shut down kitchen leds according to movement
    hal.animations.kitchen.playing = state


@hal.on_trigger('bell', True)
@sentry_listen
def bell_pressed(name, state):
    # Play Funky town on the buzzer
    with SafeBuzzer() as buz:
        buz.looping = False
        buz.upload(funkytown.to_frames() * 2)
        buz.fps = 20

        # Upload glow to the space invader eyes
        hal.animations.bell_eyes.upload(sinusoid(100))

        # Make animations much faster for 5 seconds
        original_fps = {}
        for a in ['bell_eyes', 'red', 'blue', 'green', 'kitchen']:
            original_fps[a] = hal.animations[a].fps
            hal.animations[a].fps = 200

        buz.playing = True

        publish_hal_event('bell', "On sonne à la porte")
        yield from asyncio.sleep(7)

        # Restore original animations
        for a, fps in original_fps.items():
            hal.animations[a].fps = fps
        if hal.triggers.knife_switch.on:
            hal.animations.bell_eyes.upload([1.0])


@hal.on_trigger('knife_switch')
@sentry_listen
def change_status_lechbot(trigger, state):
    if state:
        text = "Le hackerspace est ouvert ! Rainbows /o/"
    else:
        text = "Le hackerspace est fermé !"
    publish_hal_event('space_status', text)


@hal.on_trigger('knife_switch')
@sentry_listen
def change_status_spaceapi(trigger, state):
    response_incubator = yield from aiohttp.post(INCUBATOR_STATUS_CHANGE_URL, data={
        "secret": INCUBATOR_SECRET,
        "open": 1 if state else 0,
    })

    yield from response_incubator.release()


@hal.on_trigger('knife_switch', True)
@sentry_listen
def open_urlab(*args):
    set_urlab_open()
    with SafeBuzzer() as buz:
        buz.looping = False
        buz.fps = 30
        buz.upload(openmusic.to_frames())
        buz.playing = True
        yield from asyncio.sleep(3)


@hal.on_trigger('knife_switch', False)
@sentry_listen
def close_urlab(*args):
    yield from execute_command('mpc', 'pause')
    set_urlab_closed(
        switchs_on=['power', 'leds_stairs'],
        anims_fixed=['green'])
    yield from asyncio.sleep(LIGHT_TIMEOUT)

    # If the hackerspace is still closed, cut power
    if not hal.triggers.knife_switch.on:
        hal.switchs.power.on = False


@hal.on_trigger('passage', True)
@sentry_listen
def passage(*args):
    if hal.triggers.knife_switch.on:
        flash = hal.animations.door_green
        flash.looping = False
        flash.upload(sinusoid(100)[:75])
        flash.playing = True
        flash.fps = 150
    else:
        publish_hal_event('passage', "Tiens, du mouvement à l'intérieur du Hackerspace !?")


@hal.on_trigger('kitchen_move', True)
@sentry_listen
def passage_kitchen(*args):
    if not hal.triggers.knife_switch.on:
        publish_hal_event('kitchen_move', "Ça bouge dans la cuisine")


@hal.on_trigger('door_stairs', True)
@sentry_listen
def passage_stairs(*args):
    if hal.triggers.knife_switch.on:
        return

    hal.switchs.power.on = True
    hal.switchs.leds_stairs.on = True
    hal.animations.green.upload([1.0])
    hal.animations.green.looping = True
    hal.animations.green.playing = True
    hal.rgbs.knife_leds.css = '#00f'

    publish_hal_event('door_stairs', "Y'a du passage dans l'escalier")
    yield from asyncio.sleep(LIGHT_TIMEOUT/2)
    hal.rgbs.knife_leds.css = '#f0f'

    yield from asyncio.sleep(LIGHT_TIMEOUT/2)

    if not hal.triggers.knife_switch.on:
        hal.rgbs.knife_leds.css = '#000'
        hal.switchs.power.on = False
        hal.switchs.leds_stairs.on = False
        hal.animations.green.looping = False


@hal.on_trigger('button_play', True)
@sentry_listen
def toggle_mpd_play(*args):
    # mpc toggle
    yield from execute_command('mpc', 'toggle')


@hal.on_trigger('button_up', True)
@sentry_listen
def increase_mpd_volume(*args):
    # mpc volume +5
    yield from execute_command('mpc', 'volume', '+5')


@hal.on_trigger('button_down', True)
@sentry_listen
def decrease_mpd_volume(*args):
    # mpc volume -5
    yield from execute_command('mpc', 'volume', '-5')


@hal.on_trigger()
@sentry_listen
def communicate_triggers(name, state):
    """Send all triggers to influxdb"""
    payload = '%s value=%d' % (name, state)
    try:
        response = yield from aiohttp.post(INFLUX_URL, data=payload.encode(), headers={'Accept-encoding': 'identity'})
        yield from response.release()
    except Exception as err:
        logger.exception("Error while communicating trigger to influxdb")
        sentry.captureException()


### Periodic tasks ###

@asyncio.coroutine
@sentry_listen
def set_red_fps():
    # Red ledstrip frequency follow the number of people in the space
    response = yield from aiohttp.request('GET', PAMELA_URL)
    content = yield from response.content.read()
    yield from response.release()

    pamela_data = json.loads(content.decode())
    color = len(pamela_data.get('users', []))
    grey = len(pamela_data.get('unknown_mac', []))
    new_fps = 25 * log(2 + color + grey)
    hal.animations.red.fps = new_fps
    logger.info("Set red ledstrip to %d fps" % new_fps)


@asyncio.coroutine
@sentry_listen
def communicate_sensors():
    """Send sensors values to influx"""
    payload = "\n".join(
        '%s value=%f' % (s.name, s.value) for s in hal.sensors.values())
    payload += "\ntx_bytes value=%d\nrx_bytes value=%d" % (
        hal.tx_bytes, hal.rx_bytes)

    response = yield from aiohttp.post(INFLUX_URL, data=payload.encode(),
                                       headers={'Accept-encoding': 'identity'})
    yield from response.release()


@asyncio.coroutine
@sentry_listen
def hal_periodic_tasks(period_seconds=15):
    while True:
        try:
            yield from set_red_fps()
            yield from communicate_sensors()
            temp_heater = hal.sensors.temp_radiator.value
            hal.animations.heater.upload(sinusoid(val_max=temp_heater))
        except Exception as err:
            logger.exception("Error in periodic tasks")
            sentry.captureException()
        finally:
            yield from asyncio.sleep(period_seconds)


@asyncio.coroutine
@sentry_listen
def blinking_eyes():
    left = False
    while True:
        try:
            delay = 16 ** (1 - hal.sensors.light_inside.value)
            logger.info("Set blinking eyes delay to %.1f seconds" % delay)
            for i in range(30):
                hal.switchs.belgaleft.on = left
                hal.switchs.belgaright.on = not left
                left = not left
                yield from asyncio.sleep(delay)
        except Exception as err:
            logger.exception("Error in blinking eyes:")
            sentry.captureException()
            yield from asyncio.sleep(5)


class HALApp(ApplicationSession):
    @asyncio.coroutine
    def onJoin(self, details):
        if hal.triggers.knife_switch.on:
            logger.info("Hackerspace is opened")
            set_urlab_open()
        else:
            logger.info("Hackerspace is closed")
            set_urlab_closed()

        def pubevent(key, text):
            print("=========== PUBLISH EVENT", key, text)
            now = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
            self.publish('hal.eventstream', key=key, text=text, time=now)
        globals()['publish_hal_event'] = pubevent

        @asyncio.coroutine
        def on_notification(key, time, text):
            if key == 'trash':
                with SafeBuzzer() as buz:
                    buz.looping = False
                    buz.upload(trashmusic.to_frames())
                    buz.fps = 17
                    buz.playing = True
                    yield from asyncio.sleep(10)
            elif key == 'poke':
                with SafeBuzzer() as buz:
                    fps, partition = random.choice(pokemusics)
                    buz.fps = fps
                    buz.looping = False
                    buz.frames = partition.to_frames()
                    buz.fps = 30
                    buz.playing = True
                    yield from asyncio.sleep(10)

        asyncio.async(hal_periodic_tasks(15))
        asyncio.async(blinking_eyes())
        yield from self.subscribe(on_notification, u'lechbot.notifstream')
        hal.install_loop()

if __name__ == "__main__":
    try:
        runner = ApplicationRunner(WAMP_HOST, WAMP_REALM,
                               debug_wamp=False, debug=False)
        runner.run(HALApp)
    except Exception as err:
        sentry.captureException()
        raise
