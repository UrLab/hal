#include "hal.h"

#define N_TRIGGERS (sizeof(triggers)/sizeof(Trigger))
Trigger triggers[] = {
    Trigger("door_stairs", 0, 9, LOW),
    Trigger("bell", 1, 4, HIGH),
    Trigger("knife", 2, 11, HIGH)
};

#define N_SWITCHS (sizeof(switchs)/sizeof(Switch))
Switch switchs[] = {
    Switch("power", 0, 2)
};

#define N_ANIMATIONS (sizeof(animations)/sizeof(Animation))
Animation animations[] = {
    Animation("red", 0),
    Animation("green", 1),
    Animation("blue", 2)
};

#define N_SENSORS (sizeof(sensors)/sizeof(Sensor))
Sensor sensors[] = {
    Sensor("temp_radiator", 0),
    Sensor("temp_ambiant", 1),
    Sensor("light_inside", 2),
    Sensor("light_outside", 3)
};

void setup(){
    Serial.begin(115200);
}

unsigned long int last_com=0, last_ping=0;
int i;
char c, d;

void com(){
    c = Serial.read();
    
    switch (c){
        case '?':
            Serial.println("?0123456789abcdef0123456789abcdef01234567");
            break;

        case '$':
            Serial.print("$A");
            Serial.println(N_ANIMATIONS, DEC);
            for (i=0; i<N_ANIMATIONS; i++){
                Serial.print("$");
                Serial.println(animations[i].name());
            }

            Serial.print("$T");
            Serial.println(N_TRIGGERS, DEC);
            for (i=0; i<N_TRIGGERS; i++){
                Serial.print("$");
                Serial.println(triggers[i].name());
            }

            Serial.print("$C");
            Serial.println(N_SENSORS, DEC);
            for (i=0; i<N_SENSORS; i++){
                Serial.print("$");
                Serial.println(sensors[i].name());
            }

            Serial.print("$S");
            Serial.println(N_SWITCHS, DEC);
            for (i=0; i<N_SWITCHS; i++){
                Serial.print("$");
                Serial.println(switchs[i].name());
            }
            break;

        case 'S':
            while (! Serial.available());
            c = Serial.read();
            while (! Serial.available());
            d = Serial.read();
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


        case 'T':
            while (! Serial.available());
            c = Serial.read();
            if (c < N_TRIGGERS){
                Serial.print("T");
                Serial.print(c, DEC);
                Serial.println(triggers[c].isActive() ? "1" : "0");
            }
            break;
    }

    last_com = millis();
}

Switch & power = switchs[0];

void loop(){
    unsigned long int now = millis();
    if (now - last_com > 750 && now - last_ping > 1000){
        Serial.println("*");
        last_ping = now;
    }
    if (now - last_com > 1000)
        power.deactivate();

    if (Serial.available())
        com();

    for (i=0; i<N_TRIGGERS; i++)
        triggers[i].check();
}
