#include "hal.h"


Trigger triggers[] = {
    Trigger("door_stairs", 51, LOW, 500),
    Trigger("bell", 49, HIGH),
    Trigger("knife_switch", 53, HIGH, 5000),
    Trigger("passage", 28, LOW),
    Trigger("heater", 47, LOW),
    Trigger("button_up", 43, HIGH),
    Trigger("button_down", 41, HIGH),
    Trigger("button_play", 39, HIGH),
    Trigger("kitchen_move", 31, HIGH, 5000)
};

Switch switchs[] = {
    Switch("power", 22),
    Switch("leds_stairs", 24),
    Switch("ampli", 26),
    Switch("belgaleft", 52),
    Switch("belgaright", 50),
};

Animation animations[] = {
    Animation("red", 3),
    Animation("door_green", 5),
    Animation("green", 4),
    Animation("blue", 2),
    Animation("heater", 6),
    Animation("kitchen", 7),
    Animation("buzzer", 45, true),
    Animation("bell_eyes", 13),
    Animation("belgatop", 9)
};

Sensor sensors[] = {
    Sensor("temp_radiator", 3),
    Sensor("temp_ambiant", 1),
    Sensor("light_inside", 0),
    Sensor("light_outside", 2)
};

Rgb rgbs[] = {
    Rgb("knife_leds", false, 23, 25, 27),
    Rgb("roof", true, 10, 11, 8)
};


Switch & power_supply = switchs[0];
HAL_CREATE(hal, sensors, triggers, switchs, animations, rgbs);

int WATCHDOG_QUESTION_PIN = 27;
int WATCHDOG_ANSWER_PIN   = 29;

void setup(){
    hal.setup();
    pinMode(WATCHDOG_QUESTION_PIN, INPUT);
    pinMode(WATCHDOG_ANSWER_PIN, OUTPUT);
}

void loop(){
    digitalWrite(WATCHDOG_ANSWER_PIN, digitalRead(WATCHDOG_QUESTION_PIN));
    hal.loop();

    /* Power supply off if no communication */
    if (hal.last_com_delay() > 2500)
       power_supply.deactivate();
}
