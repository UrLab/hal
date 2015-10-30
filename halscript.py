import asyncio
import aiohttp
from asyncio import subprocess
from math import log
from datetime import datetime
import json

from halpy import HAL
from halpy.generators import sinusoid, Partition, Note, Silence
from internet import lechbot_notif_consume, lechbot_event
from config import HALFS_ROOT, STATUS_CHANGE_URL, PAMELA_URL, INFLUX_URL


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

pokemusic = Partition(
    Note(440), Note(415), Note(440), Note(493), Note(523), Note(587), Note(659),
    Note(440), Note(493), Note(523), Note(587), Note(659), Note(698), Note(659),
    Note(698), Note(783), Note(880), Note(698), Note(659, 3))

hal = HAL(HALFS_ROOT)


class SafeBuzzer():
    """
    Buzzer and roof red ledstrip could not be on at the same time
    (hardware limitation).
    """
    def __enter__(self):
        self.red_playing = hal.animations.roof_r.playing
        if self.red_playing:
            hal.animations.roof_r.playing = False
        return hal.animations.buzzer

    def __exit__(self, *args, **kwargs):
        if self.red_playing:
            hal.animations.roof_r.playing = True


@asyncio.coroutine
def execute_command(*args):
    proc = yield from asyncio.create_subprocess_exec(
        *args,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout, _ = yield from proc.communicate()
    exit_code = yield from proc.wait()
    return exit_code


def set_urlab_open():
    for sw in ['power', 'ampli', 'knife_g', 'leds_stairs']:
        hal.switchs[sw].on = True

    for sw in ['knife_r', 'knife_b']:
        hal.switchs[sw].on = False

    # Put fixed animations on
    for a in ['bell_eyes', 'roof_r', 'roof_g', 'roof_b']:
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

    for anim in hal.animations.values():
        if anim.name not in anims_fixed:
            anim.playing = False
        else:
            anim.upload([1.0])
            anim.looping = True
            anim.playing = True


@hal.on_trigger('heater')
def heater_changed(name, state):
    # light or shut down the heater ledstrip according to the valve
    hal.animations.heater.playing = state


@hal.on_trigger('kitchen_move')
def kitchen_changed(name, state):
    # light or shut down kitchen leds according to movement
    hal.animations.kitchen.playing = state


@hal.on_trigger('bell', True)
def bell_pressed(name, state):
    # Play Funky town on the buzzer
    with SafeBuzzer() as buz:
        buz.looping = False
        buz.upload(funkytown.to_frames()*2)
        buz.fps = 20

        # Upload glow to the space invader eyes
        hal.animations.bell_eyes.upload(sinusoid(100))

        # Make animations much faster for 5 seconds
        original_fps = {}
        for a in ['bell_eyes', 'red', 'blue', 'green', 'kitchen']:
            original_fps[a] = hal.animations[a].fps
            hal.animations[a].fps = 200

        buz.playing = True

        yield from lechbot_event('bell')
        yield from asyncio.sleep(7)

        # Restore original animations
        for a, fps in original_fps.items():
            hal.animations[a].fps = fps
        if hal.triggers.knife_switch.on:
            hal.animations.bell_eyes.upload([1.0])


@hal.on_trigger('knife_switch')
def change_status_lechbot(trigger, state):
    event = 'hs_open' if state else 'hs_close'
    yield from lechbot_event(event)


@hal.on_trigger('knife_switch')
def change_status_spaceapi(trigger, state):
    status = "open" if state else "close"
    response = yield from aiohttp.get(STATUS_CHANGE_URL + "?status=" + status)
    yield from response.release()


@hal.on_trigger('knife_switch', True)
def open_urlab(*args):
    set_urlab_open()
    with SafeBuzzer() as buz:
        buz.looping = False
        buz.fps = 30
        buz.upload(openmusic.to_frames())
        buz.playing = True
        yield from asyncio.sleep(3)


@hal.on_trigger('knife_switch', False)
def close_urlab(*args):
    yield from execute_command('mpc', 'pause')
    set_urlab_closed(
        switchs_on=['power', 'leds_stairs', 'knife_r'],
        anims_fixed=['green', 'roof_r'])
    yield from asyncio.sleep(LIGHT_TIMEOUT)

    # If the hackerspace is still closed, cut power
    if not hal.triggers.knife_switch.on:
        hal.switchs.power.on = False


@hal.on_trigger('passage', True)
def passage(*args):
    if hal.triggers.knife_switch.on:
        flash = hal.animations.door_green
        flash.looping = False
        flash.upload(sinusoid(100)[:75])
        flash.playing = True
        flash.fps = 150
    else:
        yield from lechbot_event('passage')


@hal.on_trigger('kitchen_move', True)
def passage_kitchen(*args):
    if not hal.triggers.knife_switch.on:
        yield from lechbot_event('kitchen_move')


@hal.on_trigger('door_stairs', True)
def passage_stairs(*args):
    if hal.triggers.knife_switch.on:
        return

    hal.switchs.power.on = True
    hal.switchs.leds_stairs.on = True
    hal.animations.green.upload([1.0])
    hal.animations.green.looping = True
    hal.animations.green.playing = True
    hal.switchs.knife_b.on = True

    yield from lechbot_event('door_stairs')
    yield from asyncio.sleep(LIGHT_TIMEOUT)
    hal.switchs.knife_b.on = False

    if not hal.triggers.knife_switch.on:
        hal.switchs.power.on = False
        hal.switchs.leds_stairs.on = False
        hal.animations.green.looping = False


@hal.on_trigger('button_play', True)
def toggle_mpd_play(*args):
    # mpc toggle
    yield from execute_command('mpc', 'toggle')


@hal.on_trigger('button_up', True)
def increase_mpd_volume(*args):
    # mpc volume +5
    yield from execute_command('mpc', 'volume', '+5')


@hal.on_trigger('button_down', True)
def decrease_mpd_volume(*args):
    # mpc volume -5
    yield from execute_command('mpc', 'volume', '-5')


@hal.on_trigger()
def communicate_triggers(name, state):
    """Send all triggers to influxdb"""
    payload = '%s value=%d' % (name, state)
    try:
        response = yield from aiohttp.post(INFLUX_URL, data=payload.encode())
        yield from response.release()
    except Exception as err:
        print("Error in trigger communication:", err)


@asyncio.coroutine
def on_lechbot_notif(notif_name):
    if notif_name == 'trash':
        with SafeBuzzer() as buz:
            buz.looping = False
            buz.upload(trashmusic.to_frames())
            buz.fps = 17
            buz.playing = True
            yield from asyncio.sleep(10)
    elif notif_name == 'poke':
        with SafeBuzzer() as buz:
            buz.looping = False
            buz.upload(pokemusic.to_frames())
            buz.fps = 30
            buz.playing = True
            yield from asyncio.sleep(10)


@asyncio.coroutine
def set_red_fps():
    # Red ledstrip frequency follow the number of people in the space
    response = yield from aiohttp.request('GET', PAMELA_URL)
    content = yield from response.content.read()
    yield from response.release()

    pamela_data = json.loads(content.decode())
    color = len(pamela_data.get('color', []))
    grey = len(pamela_data.get('grey', []))
    hal.animations.red.fps = 25 * log(2 + color + grey)
    print(datetime.now(), "Set fps to", 25 * log(2 + color + grey))


@asyncio.coroutine
def communicate_sensors():
    """Send sensors values to influx"""
    payload = "\n".join(
        '%s value=%f' % (s.name, s.value) for s in hal.sensors.values())
    response = yield from aiohttp.post(INFLUX_URL, data=payload.encode())
    yield from response.release()


@asyncio.coroutine
def hal_periodic_tasks(period_seconds=15):
    while True:
        try:
            yield from set_red_fps()
            yield from communicate_sensors()
            temp_heater = hal.sensors.temp_radiator.value
            hal.animations.heater.upload(sinusoid(val_max=temp_heater))
        except Exception as err:
            print("Error in periodic tasks:", err)
        finally:
            yield from asyncio.sleep(period_seconds)


@asyncio.coroutine
def blinking_eyes():
    left = False
    while True:
        delay = 2 ** hal.sensors.light_inside.value
        print("Delay", delay)
        for i in range(30):
            hal.switchs.belgaleft.on = left
            hal.switchs.belgaright.on = not left
            left = not left
            yield from asyncio.sleep(delay)


def main():
    if hal.triggers.knife_switch.on:
        print("Hackerspace is opened")
        set_urlab_open()
    else:
        print("Hackerspace is closed")
        set_urlab_closed()

    loop = asyncio.get_event_loop()
    asyncio.async(lechbot_notif_consume(on_lechbot_notif))
    asyncio.async(hal_periodic_tasks(15))
    asyncio.async(blinking_eyes())
    hal.run(loop)

if __name__ == "__main__":
    main()
