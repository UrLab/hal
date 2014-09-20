#include "hal.h"

#define N_TRIGGERS (sizeof(triggers)/sizeof(Trigger))
Trigger triggers[] = {
    Trigger("door_stairs", LOW, 9),
    Trigger("bell", HIGH, 4),
    Trigger("knife", HIGH, 11)
};

#define N_SWITCHS (sizeof(switchs)/sizeof(Switch))
Switch switchs[] = {
    Switch("power", 2)
};

#define N_ANIMATIONS (sizeof(animations)/sizeof(Animation))
Animation animations[] = {
    Animation("red"),
    Animation("green"),
    Animation("blue")
};

#define N_SENSORS (sizeof(sensors)/sizeof(Sensor))
Sensor sensors[] = {
    Sensor("temp_radiator"),
    Sensor("temp_ambiant"),
    Sensor("light_inside"),
    Sensor("light_outside")
};

unsigned long int last_com = 0;

void setup(){
    Serial.begin(115200);
}

int i;
char c;
void loop(){
    while (! Serial.available());
    c = Serial.read();
    last_com = millis();
    
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
    }
}
