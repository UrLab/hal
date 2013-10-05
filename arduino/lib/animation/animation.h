#ifndef DEFINE_ANIMATION_HEADER
#define DEFINE_ANIMATION_HEADER

#ifndef TEST_MODE
#include "Arduino.h"
#else
#include <fakeduino.h>
#endif

class Animation {
	private:
		int __pin;
		unsigned char __len; 
		unsigned char __frame_index;
		unsigned char __delay;
		unsigned long __t;
		unsigned char *__curve;
	public:
		Animation(int pin);
		Animation(int pin, unsigned char len, unsigned char *curve, unsigned char delay=25);
		~Animation();
		unsigned char length() const;
		unsigned char delay() const;
		unsigned char total_time() const {return length()*delay();}
		void seek(unsigned char position);
		void setLength(unsigned char length);
		void set_delay(unsigned char delay);
		void play();
};

class BufferedAnimation : public Animation {
	private:
		unsigned char __buffer[256];
	public:
		explicit BufferedAnimation(int pin);
		unsigned char & operator[](unsigned char offset);
};

#endif
