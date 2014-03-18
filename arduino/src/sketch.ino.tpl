/**
 * AmbianceDuino.ino
 * -----------------
 * iTitou @ UrLab
 * Released under the Creative Commons CC-BY 3.0 license.
 * http://creativecommons.org/licenses/by/3.0/
 */

#include "animation.h"
#include "Trigger.h"

#define BAUDS 115200
#define waitSerial() while(Serial.available()==0)

/* ==== pinout ==== */
#define POWER_PIN 2      //!< External power for leds, relays, ...
#define BELL_PIN 4       //!< Entrance door bell
#define LEDS_R_PIN 5     //!< Red ledstrip (around tools panel)
#define LEDS_G_PIN 6     //!< Green ledstrip (inside door)
#define BUZZER_PIN 8     //!< Buzzer (tone for bell)
#define DOOR_PIN 9       //!< Stairs door (HIGH=closed, LOW=opened)
#define LEDS_B_PIN 10    //!< Blue leds (in cableway)
#define PASSAGE_PIN 12   //!< Passage counter (HIGH=no, LOW=passage)
#define RADIATOR_PIN 13  //!< Radiator position (HIGH=closed, LOW=warm)

/*! Analog pinout */
static const char *analog_map[6] = {
    "temp_radia", "light_out", "temp_amb", "light_in", "temp_lm35", "Analog5" 
};

/*! Running states */
enum State {
    STANDBY=0,  //!< Nearly nothing to do
    POWERED=1,  //!< External power on (stairs ledstrip on)
    SHOWTIME=2, //!< Hackerspace opened
    ALERT=3     //!< Ledstrips animations in alert mode (bell)
};

#define STATE_MAX ALERT

/* Global state */
State state;

static uint8_t ringtone_notes[] = {
    0, 123, 0, 123, 0, 110, 0, 123, 123, 123, 0, 92, 92, 0, 0, 92, 0, 123, 
    0, 164, 0, 155, 0, 123, 123, 123, 123, 123, 123, 123, 123, 123
};

static Unit door_flash[] = {
    196, 213, 227, 239, 247, 253, 254, 253, 248, 239, 228, 214, 197, 179, 
    159, 138, 117, 96, 76, 57, 41, 26, 15, 6, 1
};

Trigger bell_trigger(BELL_PIN, LOW, 20000, "bell");
Trigger passage_trigger(PASSAGE_PIN, HIGH, 1000, "passage", 20);
Trigger door_trigger(DOOR_PIN, HIGH, 60000, "door");
Trigger radiator_trigger(RADIATOR_PIN, HIGH, 10000, "radiator", 20);

DCLedstrip<255> ledstrip_r(LEDS_R_PIN, 40);
DCLedstrip<sizeof(door_flash)/sizeof(Unit)> ledstrip_g(LEDS_G_PIN, 2*sizeof(door_flash));
DCLedstrip<255> ledstrip_ringtone(LEDS_R_PIN, 500);

#define N_LEDSTRIPS sizeof(ledstrips)/sizeof(void*)
DCLedstrip<255> *ledstrips[] = {&ledstrip_r, &ledstrip_ringtone};

/* ==== Subroutines ==== */
void setup_STANDBY()
{
    Serial.println("SSTANDBY");
    digitalWrite(POWER_PIN, LOW);
}

void setup_POWERED()
{
    Serial.println("SPOWERED");
    digitalWrite(POWER_PIN, HIGH);
    ledstrip_r.off();
    ledstrip_g.off();
    ledstrip_ringtone.off();
    radiator_trigger.deactivate(); /* Will raise again if physically active */
}

void setup_SHOWTIME()
{
    Serial.println("SSHOWTIME");
    digitalWrite(POWER_PIN, HIGH);
    ledstrip_g.on();
    ledstrip_r.on();
    ledstrip_ringtone.off();
}

void setup_ALERT()
{
    Serial.println("SALERT");
    digitalWrite(POWER_PIN, HIGH);
    ledstrip_r.off();
    ledstrip_ringtone.on();
}

void loop_STANDBY()
{
    if (door_trigger.isActive()){
        state = POWERED;
    }
    radiator_trigger.isActive();
}

void loop_POWERED()
{
    if (! door_trigger.isActive()){
        state = STANDBY;
    }
}

void loop_SHOWTIME()
{
    if (bell_trigger.isActive()){
        state = ALERT;
    } else {
        ledstrip_r.play();
        if (passage_trigger.isActive())
            ledstrip_g.play();
        else
            ledstrip_g.off();
    }
}

void loop_ALERT()
{
    if (! bell_trigger.isActive())
        state = SHOWTIME;
    else {
        ledstrip_ringtone.play();
    }
}

typedef void(*Func)(void);
Func transitions[] = {
    setup_STANDBY, setup_POWERED, setup_SHOWTIME, setup_ALERT
};

Func mainloops[] = {
    loop_STANDBY, loop_POWERED, loop_SHOWTIME, loop_ALERT
};

/* ==== Serial communication ==== */
static void read_serial(){
    if (Serial.available()){
        char command = Serial.read();
        char index, n;
        switch (command){
            case '?':
                Serial.println("?{{version}}");
                break;
            
            case '-': 
                state = SHOWTIME;
                Serial.println("-");
                break;

            case '_': 
                door_trigger.force_activate();
                state = POWERED; 
                Serial.println("_");
                break;
            
            case '@':
                Serial.print("@{");
                for (char i=0; i<6; i++){
                    if (i>0) Serial.print(",");
                    Serial.print("\"");
                    Serial.print(analog_map[i]);
                    Serial.print("\":");
                    Serial.print(analogRead(i), DEC);
                }
                Serial.println("}");
                break;
            
            /*! Syntax: #<i><fps>, where i is the index of the anim, and fps 
                        the desired FPS. Both as unsigned bytes */
            case '#':
                while (! Serial.available());
                index = Serial.read();
                while (! Serial.available());
                n = Serial.read();

                if (index < N_LEDSTRIPS){
                    ledstrips[index]->setFPS(n);
                    Serial.println("#");
                } else {
                    Serial.println("!");
                }
                break;
            
            /*! Syntax: %<i>where i is the index of the anim as unsigned byte */
            case '%':
                /* Reset anim to default */
                Serial.println("%");
                break;
            
            /*! Syntax: U<i><n><...> where i is the index of the anim, n the 
                        length of the animations (<=255), and the rest are the
                        bytes of the animation. All as unsigned bytes. */
            case 'U':
                while (! Serial.available());
                index = Serial.read();
                while (! Serial.available());
                n = Serial.read();

                if (index < N_LEDSTRIPS){
                    ledstrips[index]->setLen(n);
                    Serial.println("U");
                } else {
                    Serial.println("!");
                }

                for (int i=0; i<n; i++){
                    while (! Serial.available());
                    unsigned char val = Serial.read();
                    if (index < N_LEDSTRIPS)
                        (*(ledstrips[index]))[i] = val;
                }

                break;
        }
    }
}

/* ==== Arduino main ==== */
void setup()
{
    Serial.begin(BAUDS);

    state = STANDBY;

    /* Setting default sinusoid for all ledstrips */
    ledstrip_r.resetDefault();
    ledstrip_ringtone.resetDefault();
    for (Unit i=0; i<ledstrip_g.len(); i++)
        ledstrip_g[i] = door_flash[i];
}

void loop()
{
    State old_state = state;
    read_serial();

    if (state == old_state && state <= STATE_MAX){
        mainloops[state]();
    }

    if (state != old_state && state <= STATE_MAX){
        transitions[state]();
    }
}
