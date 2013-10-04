/**
 * AmbianceDuino.ino
 * -----------------
 * iTitou @ UrLab
 * Released under the Creative Commons CC-BY 3.0 license.
 * http://creativecommons.org/licenses/by/3.0/
 */

#include "animation.h"

/* ==== pinout ==== */
#define POWER1 2
#define LEDS 3

/* ==== Subroutines ==== */
static void update_ledstrips();
static void read_serial();

static int i, c;
static unsigned char anim_buffer[256];
BufferedAnimation ledstrip(LEDS);

/* ==== Arduino main ==== */
void setup(){
	Serial.begin(115200);
	pinMode(13, OUTPUT);
}

void loop(){
	read_serial();
	update_ledstrips();
}

/* ==== Ledstrips animations ==== */

static bool ledstrip_power = false;

static void update_ledstrips(){
	digitalWrite(POWER1, (ledstrip_power) ? HIGH : LOW);
	if (ledstrip_power) ledstrip.play();
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
				Serial.print("@[");
				for (i=0; i<6; i++){
					if (i>0) Serial.print(",");
					Serial.print(analogRead(i), DEC);
				}
				Serial.println("]");
				break;
			case '#':
				if (Serial.available()){
					c = Serial.read();
					ledstrip.set_delay(c);
				}
				Serial.print("#");
				Serial.println(ledstrip.delay(), DEC);
				break;
			case 'R':
				delayMicroseconds(9);
				if (Serial.available()){
					int len = Serial.read();
					for (i=0; i<len; i++)
						anim_buffer[i] = Serial.read();
					Serial.println("R");
				} else {
					Serial.println("!R");
				}
		}
	}
}
