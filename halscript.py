import asyncio
from halpy import HAL
from halpy.generators import sinusoid, Partition, Note, Silence
from internet import lechbot_notif_consume, lechbot_event
from config import HALFS_ROOT

funkytown = Partition(
    Note(494), Note(494), Note(440), Note(494, 2), Note(370, 2), Note(370),
    Note(494), Note(659), Note(622), Note(494, 2), Silence(3))

trashmusic = Partition(
        Note(220), Note(262), Note(330), Note(440), Note(494), Note(262), Note(330),
        Note(494), Note(523), Note(330), Note(262), Note(523), Note(370), Note(294),
        Note(220), Note(370), Note(349), Note(262), Note(220), Note(262, 2),
        Note(392), Note(392, 0.5), Note(330, 0.5), Note(262), Note(247), Note(262),
        Note(262, 2))

hal = HAL(HALFS_ROOT)


@hal.on_trigger('heater')
def heater_changed(name, state):
    # light or shut down the heater ledstrip according to the valve
    hal.animations.heater.play = state


@hal.on_trigger('bell', True)
def bell_pressed(name, state):
    # Play Funky town on the buzzer
    buzzer = hal.animations.buzzer
    buzzer.looping = False
    buzzer.upload(funkytown)
    buzzer.fps = 17
    buzzer.playing = True

    # Upload glow to the space invader eyes
    hal.animations.bell_eyes.upload(sinusoid(100))

    # Make animations much faster for 5 seconds
    original_fps = {}
    for a in ['bell_eyes', 'red', 'blue', 'green']:
        original_fps[a] = hal.animations[a].fps
        hal.animations[a].fps = 200

    yield from lechbot_event('bell')
    yield from asyncio.sleep(5)

    # Restore original animations
    for a, fps in original_fps.items():
        hal.animations[a].fps = fps
    if hal.triggers.knife_switch.on:
        hal.animations.bell_eyes.upload([1.0])


@hal.on_trigger('knife_switch', True)
def open_urlab(*args):
    for sw in ['power', 'ampli', 'knife_g']:
        hal.switchs[sw].on = True

    for sw in ['knife_r', 'knife_b']:
        hal.switchs[sw].on = False

    # Put eyes on
    hal.animations.bell_eyes.upload([1.0])

    # Start glowing ledstrips
    for a in ['belgatop', 'blue', 'green', 'heater', 'kitchen', 'red']:
        anim = hal.animations[a]
        anim.upload(sinusoid())
        anim.fps = 40
        anim.loop = True
        anim.play = True

    heater_changed('heater', hal.triggers.heater.on)
    yield from lechbot_event('hs_open')


@hal.on_trigger('knife_switch', False)
def close_urlab(*args):
    for sw in ['knife_r']:
        hal.switchs[sw].on = True

    for sw in ['ampli', 'knife_g', 'knife_b']:
        hal.switchs[sw].on = False

    # Keep light inside to easily close UrLab
    for a in ['green', 'roof_r', 'roof_g', 'roof_b']:
        anim = hal.animations[a]
        anim.upload([1.0])

    # Stop all other lights
    for a in ['belgatop', 'blue', 'heater', 'kitchen', 'red']:
        hal.animations[a].stop()

    yield from lechbot_event('hs_close')
    yield from asyncio.sleep(60)

    # If the hackerspace is still closed, cut power
    if not hal.triggers.knife_switch.on:
        hal.switchs.power.on = False


@hal.on_trigger('passage', True)
def passage(*args):
    flash = hal.animations.green_door
    flash.looping = False
    flash.upload(sinusoid(100)[:75])
    flash.playing = True

    if not hal.triggers.knife_switch.on:
        yield from lechbot_event('passage')


def on_lechbot_notif(notif_name):
    if notif_name == 'trash':
        buzzer = hal.animations.buzzer
        buzzer.looping = False
        buzzer.upload(trashmusic)
        buzzer.fps = 17
        buzzer.playing = True

if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    asyncio.async(lechbot_notif_consume(on_lechbot_notif))
    hal.run(loop)
