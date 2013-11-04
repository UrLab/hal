/**
 * AmbianceDuino.ino
 * -----------------
 * iTitou @ UrLab
 * Released under the Creative Commons CC-BY 3.0 license.
 * http://creativecommons.org/licenses/by/3.0/
 */

#include "animation.h"

#define BAUDS 115200
#define waitSerial() while(Serial.available()==0)

/* ==== pinout ==== */
#define BELL 4
#define POWER1 2
#define LEDS_R 5
#define LEDS_G 6
#define BUZZER 8
#define PASSAGE 12

 static const char *analog_map[6] = {
 	"temp_radia", "light_out", "temp_amb", "light_in", "temp_lm35", "Analog5" 
 };

/* ==== Subroutines ==== */
static void update_ledstrips();
static void read_serial();
static void door_bell_check();

static int i, c;
BufferedAnimation ledstrip_r(LEDS_R);
BufferedAnimation ledstrip_g(LEDS_G);

/* ==== Arduino main ==== */
void setup(){
	Serial.begin(BAUDS);
	pinMode(BELL, INPUT);
}

void loop(){
	read_serial();
	update_ledstrips();
	door_bell_check();
}

/* ==== Door & bell ==== */
static bool ringtone_active = false;
static int bell_state = LOW;
static uint8_t ringtone_notes[] = {
	0, 123, 0, 123, 0, 110, 0, 123, 123, 123, 0, 92, 92, 0, 0, 92, 0, 123, 
	0, 164, 0, 155, 0, 123, 123, 123, 123, 123, 123, 123, 123, 123
};
static Animation ringtone(BUZZER, sizeof(ringtone_notes), ringtone_notes, 126);
static Animation ringtone_leds(LEDS_R, 2);

static void door_bell_check(){
	int b = digitalRead(BELL);
	if (ringtone_active){
		ringtone.play_tone();
		if (ringtone.loop() >= 2){
			ringtone_active = false;
			ringtone.reset_loop();
			noTone(BUZZER);
		}
	}
	if (b==HIGH && bell_state==LOW){
		bell_state = b;
		Serial.println("*");
		ringtone_active = true;
	} else if (b==LOW && bell_state==HIGH){
		bell_state = b;
	}
}


/* ==== Ledstrips animations ==== */
static bool ledstrip_power = false;

static void update_ledstrips(){
	digitalWrite(POWER1, (ledstrip_power) ? HIGH : LOW);
	if (ledstrip_power){
		if (ringtone_active)
			ringtone_leds.play();
		else {
			ledstrip_r.play();
		}
		if (digitalRead(PASSAGE) == LOW){
			ledstrip_g.play();
			tone(BUZZER, 1000, 200);
		} else {
			analogWrite(LEDS_G, 0);
	
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
			case 'G':
				waitSerial();
				if (Serial.available()){
					ledstrip_g.setLength(Serial.read());
					for (i=0; i<ledstrip_g.length(); i++){
						waitSerial();
						ledstrip_g[i] = Serial.read();
					}
					Serial.print("G");
					Serial.println(i);
				} else {
					Serial.println("!G");
				}
		}
	}
}
