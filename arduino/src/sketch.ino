#include "hal.h"

#define N_TRIGGERS (sizeof(triggers)/sizeof(Trigger))
Trigger triggers[] = {
    Trigger("door_stairs", 51, LOW, 500),
    Trigger("bell", 49, HIGH),
    Trigger("knife_switch", 53, HIGH, 5000),
    Trigger("passage", 12, LOW),
    Trigger("heater", 47, LOW)
};

#define N_SWITCHS (sizeof(switchs)/sizeof(Switch))
Switch switchs[] = {
    Switch("power", 22),
    Switch("leds_stairs", 24),
    Switch("ampli", 26)
};

#define N_ANIMATIONS (sizeof(animations)/sizeof(Animation))
Animation animations[] = {
    Animation("red", 3),
    Animation("door_green", 5),
    Animation("green", 4),
    Animation("blue", 2),
    Animation("heater", 6),
    Animation("kitchen", 7),
    Animation("buzzer", 45, true),
    Animation("bell_eyes", 13)
};

#define N_SENSORS (sizeof(sensors)/sizeof(Sensor))
Sensor sensors[] = {
    Sensor("temp_radiator", 3),
    Sensor("temp_ambiant", 1),
    Sensor("light_inside", 0),
    Sensor("light_outside", 2)
};

Switch & power_supply = switchs[0];
HAL_CREATE(hal, sensors, triggers, switchs, animations);

void setup(){
    hal.setup();
}

void loop(){
    hal.loop();

    /* Power supply off if no communication */
    if (hal.ping_timeout())
        power_supply.deactivate();
}
