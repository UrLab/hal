#include "hal.h"

#define N_TRIGGERS (sizeof(triggers)/sizeof(Trigger))
Trigger triggers[] = {
    Trigger("door_stairs", 51, LOW),
    Trigger("bell", 49, HIGH),
    Trigger("knife_switch", 53, HIGH),
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
    Animation("buzzer", 45, true)
};

#define N_SENSORS (sizeof(sensors)/sizeof(Sensor))
Sensor sensors[] = {
    Sensor("temp_radiator", 3),
    Sensor("temp_ambiant", 1),
    Sensor("light_inside", 0),
    Sensor("light_outside", 2)
};

void setup(){
    for (int i=0; i<30; i++){
        if (i<N_ANIMATIONS) animations[i].setID(i);
        if (i<N_SENSORS) sensors[i].setID(i);
        if (i<N_TRIGGERS) triggers[i].setID(i);
        if (i<N_SWITCHS) switchs[i].setID(i);
    }

    Serial.begin(115200);
    for (int i=0; i<N_ANIMATIONS; i++){
        animations[i].setLen(1);
        animations[i][0] = 0;
    }
}

unsigned long int now=0, last_com=0, last_ping=0, lag=0;
int j;
unsigned char c, d, e;

void com(){
    c = Serial.read();

    switch (c){
        /* Ping */
        case '*':
            lag = now - last_ping;
            break;

        /* Ask version */
        case '?':
            Serial.println("?0123456789abcdef0123456789abcdef01234567");
            break;

        /* Ask Resources */
        case '$':
            Serial.print("$A");
            Serial.println(N_ANIMATIONS, DEC);
            for (int i=0; i<N_ANIMATIONS; i++){
                Serial.print("$");
                Serial.println(animations[i].name());
            }

            Serial.print("$T");
            Serial.println(N_TRIGGERS, DEC);
            for (int i=0; i<N_TRIGGERS; i++){
                Serial.print("$");
                Serial.println(triggers[i].name());
            }

            Serial.print("$C");
            Serial.println(N_SENSORS, DEC);
            for (int i=0; i<N_SENSORS; i++){
                Serial.print("$");
                Serial.println(sensors[i].name());
            }

            Serial.print("$S");
            Serial.println(N_SWITCHS, DEC);
            for (int i=0; i<N_SWITCHS; i++){
                Serial.print("$");
                Serial.println(switchs[i].name());
            }
            break;

        /* Set switch or ask for status */
        case 'S':
            while (! Serial.available());
            c = Serial.read(); //switch id
            while (! Serial.available());
            d = Serial.read(); //switch status
            if (c < N_SWITCHS){
                switch (d){
                    case 0: switchs[c].deactivate(); break;
                    case 1: switchs[c].activate(); break;
                }
                Serial.print("S");
                Serial.print(c, DEC);
                Serial.println(switchs[c].isActive() ? "1" : "0");
            }
            break;

        /* Upload animation */
        case 'A':
            while (! Serial.available());
            c = Serial.read(); //anim id
            while (! Serial.available());
            d = Serial.read(); //number of frames

            for (j=0; j<d; j++){
                while (! Serial.available());
                e = Serial.read();
                if (c < N_ANIMATIONS)
                    animations[c][j] = e;
            }
            if (c < N_ANIMATIONS)
                animations[c].setLen(d);
            Serial.print("A");
            Serial.print(c, DEC);
            Serial.print(":");
            Serial.println(d, DEC);
            break;

        /* Animation settings */
        case 'a':
            while (! Serial.available());
            c = Serial.read(); //anim id
            while (! Serial.available());
            d = Serial.read(); //request type {'l'(oop), 'p'(lay), 'd'(elay)}
            while (! Serial.available());
            e = Serial.read(); //param

            if (c < N_ANIMATIONS){
                switch (d){
                    case 'l':
                        if (e <= 1)
                            animations[c].setLoop(e);
                        Serial.print("a");
                        Serial.print(c, DEC);
                        Serial.println(animations[c].isLoop() ? ":l1" : ":l0");
                        break;

                    case 'p':
                        if (e <= 1)
                            animations[c].play(! e);
                        Serial.print("a");
                        Serial.print(c, DEC);
                        Serial.println(animations[c].isPlaying() ? ":p1" : ":p0");
                        break;

                    case 'd':
                        if (e > 0)
                            animations[c].setDelay(e);
                        Serial.print("a");
                        Serial.print(c, DEC);
                        Serial.print(":d");
                        Serial.println(animations[c].getDelay(), DEC);
                        break;
                }
            }
            break;

        /* Ask for trigger status */
        case 'T':
            while (! Serial.available());
            c = Serial.read(); //trigger id
            if (c < N_TRIGGERS){
                Serial.print("T");
                Serial.print(c, DEC);
                Serial.println(triggers[c].isActive() ? "1" : "0");
            }
            break;

        /* Ask for sensor value */
        case 'C':
            while (! Serial.available());
            c = Serial.read(); //sensor id
            if (c < N_SENSORS){
                Serial.print("C");
                Serial.print(c, DEC);
                Serial.print(":");
                Serial.println(sensors[c].getValue());
            }
            break;
    }

    last_com = millis();
}

Switch & power = switchs[0];

void loop(){
    now = millis();

    /* Ping every second if no communication within last 750ms */
    if (now - last_com > 750 && now - last_ping > 1000){
        Serial.println("*");
        last_ping = now;
    }

    /* Power supply off if no communication within last 1.5s */
    if (now - last_com > 2500)
        power.deactivate();

    if (Serial.available())
        com();

    for (int i=0; i<N_TRIGGERS; i++)
        triggers[i].check();
    for (int i=0; i<N_ANIMATIONS; i++)
        animations[i].run(now);
    for (int i=0; i<N_SWITCHS; i++)
        switchs[i].writeVal();
}
