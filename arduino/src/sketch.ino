#include "hal.h"


Trigger triggers[] = {
    Trigger("door_stairs", 51, LOW, 500),
    Trigger("bell", 49, HIGH),
    Trigger("knife_switch", 53, HIGH, 5000),
    Trigger("passage", 28, LOW),
    Trigger("heater", 47, LOW)
};

Switch switchs[] = {
    Switch("power", 22),
    Switch("leds_stairs", 24),
    Switch("ampli", 26)
};

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
    if (hal.last_com_delay() > 2500)
       power_supply.deactivate();

    // Serial.print("Last com delay: ");
    // Serial.println(hal.last_com_delay(), DEC);
}
