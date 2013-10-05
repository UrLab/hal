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
#define LEDS 3
#define BUZZER 8

 static const char *analog_map[6] = {
 	"temp_radia", "light_out", "temp_amb", "light_in", "temp_lm35", "Analog5" 
 };

/* ==== Subroutines ==== */
static void update_ledstrips();
static void read_serial();
static void door_bell_check();

static int i, c;
BufferedAnimation ledstrip(LEDS);

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
static int bell_state = LOW;
static void door_bell_check(){
	int b = digitalRead(BELL);
	if (b==HIGH && bell_state==LOW){
		bell_state = b;
		Serial.println("*");
		tone(BUZZER, 440, 1000);
	} else if (b==LOW && bell_state==HIGH){
		bell_state = b;
	}
}


/* ==== Ledstrips animations ==== */
static bool ledstrip_power = false;

static void update_ledstrips(){
	digitalWrite(POWER1, (ledstrip_power) ? HIGH : LOW);
	if (ledstrip_power) ledstrip.play();
	else analogWrite(LEDS, 0);
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
				if (c != 0)	ledstrip.set_delay(c);
				Serial.print("#");
				Serial.println(ledstrip.delay(), DEC);
				break;
			case 'R':
				waitSerial();
				if (Serial.available()){
					ledstrip.setLength(Serial.read());
					for (i=0; i<ledstrip.length(); i++){
						waitSerial();
						ledstrip[i] = Serial.read();
					}
					Serial.print("R");
					Serial.println(i);
				} else {
					Serial.println("!R");
				}
		}
	}
}
