#ifndef DEFINE_TRIGGER_HEADER
#define DEFINE_TRIGGER_HEADER

#include "Arduino.h"

class Trigger {
	private:
		int _pin, _idleState;
		unsigned int _active, _activeAfter;
		unsigned long int _active_from, _timeout;
		String _name;
	public:
		Trigger(int pin, int idleState, unsigned long int timeout, String name, unsigned int activeAfter=1);
		void activate(void);
		void deactivate(void);
		bool isActive(void);
		bool isIdleState(void);
};

#endif
