#include "animation.h"

const Unit predefined_sin[] = {
    127, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 168, 170, 
    173, 176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 204, 206, 209, 211, 
    213, 216, 218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 239, 240, 
    242, 243, 244, 246, 247, 248, 249, 250, 251, 251, 252, 253, 253, 254, 254, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 253, 252, 251, 
    251, 250, 249, 248, 247, 246, 244, 243, 242, 240, 239, 237, 235, 234, 232, 
    230, 228, 226, 224, 222, 220, 218, 216, 213, 211, 209, 206, 204, 201, 198, 
    196, 193, 190, 188, 185, 182, 179, 176, 173, 170, 168, 165, 162, 158, 155, 
    152, 149, 146, 143, 140, 137, 134, 131, 127, 124, 121, 118, 115, 112, 109, 
    106, 102, 99, 96, 93, 90, 87, 84, 81, 78, 76, 73, 70, 67, 64, 62, 59, 56, 
    54, 51, 49, 46, 44, 42, 39, 37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 18, 16, 
    15, 13, 12, 10, 9, 8, 7, 6, 5, 4, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 15, 16, 18, 19, 21, 23, 
    25, 27, 29, 31, 33, 35, 37, 39, 42, 44, 46, 49, 51, 54, 56, 59, 62, 64, 67, 
    70, 73, 76, 78, 81, 84, 87, 90, 93, 96, 99, 102, 106, 109, 112, 115, 118, 
    121, 124
};

Animation::Animation(unsigned int fps) : 
	_len(sizeof(predefined_sin)), 
	_default_mode(true), _frame(0), _delay(40), _last_tick(millis())
{
	setFPS(fps);
}

void Animation::setFPS(unsigned int fps)
{
	if (fps > 0)
		_delay = 1000/fps;
}

Unit & Animation::operator[](Unit i)
{
	return _curve[i];
}

Unit const & Animation::operator[](Unit i) const
{
	return _curve[i];
}

void Animation::setLength(unsigned int len)
{
	if (len <= sizeof(_curve)){
		_default_mode = false;
		_len = len;
	}
}

void Animation::on(){_on = true;}
void Animation::off(){_on = false;}

void Animation::play(unsigned long int tick)
{
	if (! _on)
		return;
	if (tick-_last_tick > _delay){
		_frame = (_frame+1)%_len;
		_last_tick = tick;
	}
	if (_default_mode)
		output(predefined_sin[_frame]);
	else
		output(_curve[_frame]);
}

void Animation::reset()
{
	_default_mode = true;
	_len = sizeof(predefined_sin);
}

Ledstrip::Ledstrip(int pin) : Animation(), _pin(pin)
{}

void Ledstrip::output(Unit val)
{
	analogWrite(_pin, val);
}

void Ledstrip::off()
{
	Animation::off();
	analogWrite(_pin, 0);
}
