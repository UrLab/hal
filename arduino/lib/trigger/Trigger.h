#ifndef DEFINE_TRIGGER_HEADER
#define DEFINE_TRIGGER_HEADER

class Trigger {
	private:
		int _pin, _idleState;
		bool _active;
		unsigned long int _active_from, _timeout;
		char _onActivate;
	public:
		Trigger(int pin, int idleState, unsigned long int timeout=0, char onActivate=0);
		void activate(void);
		void deactivate(void);
		bool isActive(void);
		bool isIdleState(void);
};

#endif
