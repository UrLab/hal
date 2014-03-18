#include "Trigger.h"
#include "Arduino.h"

Trigger::Trigger(int pin, int idleState, unsigned long int timeout, String name, unsigned int activeAfter):
_pin(pin), _idleState(idleState), _active(0), _name(name),
_active_from(0), _timeout(timeout), _activeAfter(activeAfter)
{
	pinMode(_pin, INPUT);
}

void Trigger::activate(void)
{
	_active++;
	if (_active == _activeAfter){
		_active_from = millis();
		Serial.println("T"+_name+"1");
	}
}

void Trigger::force_activate(void)
{
	_active = _activeAfter-1;
	activate();
}

void Trigger::deactivate(void)
{
	_active = 0;
	Serial.println("T"+_name+"0");
}

bool Trigger::isIdleState(void)
{
	return digitalRead(_pin) == _idleState;
}

bool Trigger::isActive(void)
{
	bool idle = isIdleState();
	bool on = _active == _activeAfter;
	if (! on && ! idle){
		activate();
	} else if (on && idle) {
		if (millis()-_active_from >= _timeout)
			deactivate();
	}
	return _active == _activeAfter;
}
