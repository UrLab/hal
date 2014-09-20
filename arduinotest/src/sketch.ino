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
    Animation("red", 0, 5),
    Animation("green", 1, 6),
    Animation("blue", 2, 10)
};

#define N_SENSORS (sizeof(sensors)/sizeof(Sensor))
Sensor sensors[] = {
    Sensor("temp_radiator", 0, 0),
    Sensor("temp_ambiant", 1, 2),
    Sensor("light_inside", 2, 3),
    Sensor("light_outside", 3, 1)
};

void setup(){
    Serial.begin(115200);
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

        /* Upload animation */
        case 'A':
            while (! Serial.available());
            c = Serial.read();
            while (! Serial.available());
            d = Serial.read();
            
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

        /* Ask for trigger status */
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
    now = millis();
    if (now - last_com > 750 && now - last_ping > 1000){
        Serial.println("*");
        last_ping = now;
    }
    if (now - last_com > 1000)
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
