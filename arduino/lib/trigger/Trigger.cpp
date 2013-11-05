#include "Trigger.h"
#include "Arduino.h"

Trigger::Trigger(int pin, int idleState, unsigned long int timeout, char onActivate):
_pin(pin), _idleState(idleState), _active(false), _onActivate(onActivate),
_active_from(0), _timeout(timeout)
{
	pinMode(_pin, INPUT);
}

void Trigger::activate(void)
{
	_active = true;
	_active_from = millis();
	if (_onActivate)
		Serial.println(_onActivate);
}

void Trigger::deactivate(void)
{
	_active = false;
}

bool Trigger::isIdleState(void)
{
	return digitalRead(_pin) == _idleState;
}

bool Trigger::isActive(void)
{
	bool idle = isIdleState();
	if (! _active && ! idle){
		activate();
	} else if (_active && idle) {
		if (millis()-_active_from >= _timeout)
			deactivate();
	}
	return _active;
}
