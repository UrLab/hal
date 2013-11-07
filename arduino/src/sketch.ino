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
#define BELL 4
#define POWER1 2
#define LEDS_R 5
#define LEDS_G 6
#define BUZZER 8
#define DOOR 9
#define PASSAGE 12

 static const char *analog_map[6] = {
 	"temp_radia", "light_out", "temp_amb", "light_in", "temp_lm35", "Analog5" 
 };

/* ==== Subroutines ==== */
static void update_ledstrips();
static void read_serial();
static void door_bell_check();

/* ==== Arduino main ==== */
void setup(){
	Serial.begin(BAUDS);
}

void loop(){
	read_serial();
	update_ledstrips();
	door_bell_check();
}

/* ==== Door & bell ==== */
static uint8_t ringtone_notes[] = {
	0, 123, 0, 123, 0, 110, 0, 123, 123, 123, 0, 92, 92, 0, 0, 92, 0, 123, 
	0, 164, 0, 155, 0, 123, 123, 123, 123, 123, 123, 123, 123, 123
};

static uint8_t door_flash[] = {
	196, 213, 227, 239, 247, 253, 254, 253, 248, 239, 228, 214, 197, 179, 
	159, 138, 117, 96, 76, 57, 41, 26, 15, 6, 1
};

static int i, c;
BufferedAnimation ledstrip_r(LEDS_R);

static Animation ringtone(BUZZER, sizeof(ringtone_notes), ringtone_notes, 126);
static Animation ringtone_leds(LEDS_R, 2);
static Animation ledstrip_g(LEDS_G, sizeof(door_flash), door_flash, 500/sizeof(door_flash));

Trigger bell_trigger(BELL, LOW, 20000, '*');
Trigger passage_trigger(PASSAGE, HIGH, 1000, 10);
Trigger door_trigger(DOOR, HIGH, 60000, '$');

static void door_bell_check(){
	if (bell_trigger.isActive()){
		ringtone.play_tone();
		if (ringtone.loop() >= 2){
			bell_trigger.deactivate();
			ringtone.reset_loop();
			noTone(BUZZER);
		}
	}
	door_trigger.isActive();
}


/* ==== Ledstrips animations ==== */
static bool ledstrip_power = false;

static void update_ledstrips(){
	digitalWrite(POWER1, (ledstrip_power) ? HIGH : LOW);
	if (ledstrip_power){
		if (bell_trigger.isActive())
			ringtone_leds.play();
		else 
			ledstrip_r.play();
		if (passage_trigger.isActive()){
			ledstrip_g.play();
			if (ledstrip_g.loop() >= 1){
				passage_trigger.deactivate();
				ledstrip_g.reset_loop();
				analogWrite(LEDS_G, 0);
			}
		}
	} else {
		analogWrite(LEDS_R, 0);
		analogWrite(LEDS_G, 0);
	}
}

/* ==== Serial communication ==== */
static void read_serial(){
	if (Serial.available()){
		switch (Serial.read()){
			case '?':
				Serial.println("?jesuisuncanapequichante");
				break;
			case '-': 
				ledstrip_power = true;  
				Serial.println("-");
				break;
			case '_': 
				ledstrip_power = false; 
				Serial.println("_");
				break;
			case '@':
				Serial.print("@{");
				for (i=0; i<6; i++){
					if (i>0) Serial.print(",");
					Serial.print("\"");
					Serial.print(analog_map[i]);
					Serial.print("\":");
					Serial.print(analogRead(i), DEC);
				}
				Serial.println("}");
				break;
			case '#':
				waitSerial();
				c = Serial.read();
				if (c != 0){
					ledstrip_r.set_delay(c);
					ledstrip_g.set_delay(c);
				}
				Serial.print("#");
				Serial.println(ledstrip_r.delay(), DEC);
				break;
			case 'R':
				waitSerial();
				if (Serial.available()){
					ledstrip_r.setLength(Serial.read());
					for (i=0; i<ledstrip_r.length(); i++){
						waitSerial();
						ledstrip_r[i] = Serial.read();
					}
					Serial.print("R");
					Serial.println(i);
				} else {
					Serial.println("!R");
				}
		}
	}
}
