#include "Trigger.h"
#include "Arduino.h"

Trigger::Trigger(int pin, int idleState, unsigned long int timeout, char onActivate, unsigned int activeAfter):
_pin(pin), _idleState(idleState), _active(0), _onActivate(onActivate),
_active_from(0), _timeout(timeout), _activeAfter(activeAfter)
{
	pinMode(_pin, INPUT);
}

void Trigger::activate(void)
{
	_active++;
	if (_active == _activeAfter){
		_active_from = millis();
		if (_onActivate)
			Serial.println(_onActivate);
	}
}

void Trigger::deactivate(void)
{
	_active = 0;
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
