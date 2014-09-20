#ifndef DEFINE_HAL_HEADER
#define DEFINE_HAL_HEADER

#include <Arduino.h>

class Resource {
    private:
        const char *_name;
    public:
        explicit Resource(const char *name) : _name(name) {}
        const char *name() {return _name;}
};

#define TRIGGER_AFTER 10
class Trigger : public Resource {
    private:
        const int _active_state; 
        int _pin, _last_state, _count_state;
    public:
        explicit Trigger(const char *name, int pin, int active_state=HIGH) : Resource(name), _pin(pin), _active_state(active_state), _last_state(LOW), _count_state(0){}
        bool isActive(){
            return _count_state >= TRIGGER_AFTER && _last_state == _active_state;
        }
        void hit(int state){
            if (state != _last_state)
                _count_state = 0;
            else if (_count_state < TRIGGER_AFTER){
                _count_state++;
                if (_count_state == TRIGGER_AFTER){
                    Serial.print("<T");
                    Serial.print(name());
                    Serial.println((state == _active_state) ? "1" : "0");
                }
            }
            _last_state = state;
        }
        void check(){hit(digitalRead(_pin));}
};

class Switch : public Resource {
    private:
        int _state, _pin;
    public:
        explicit Switch(const char *name, int pin) : Resource(name), _state(LOW), _pin(pin){}
        void activate(){_state = HIGH;}
        void deactivate(){_state = LOW;}
        int state() const {return _state;}
        void writeVal() const {digitalWrite(_pin, state());}
};

class Animation : public Resource {
    public:
        Animation(const char *name) : Resource(name){}
};

class Sensor : public Resource {
    public:
        Sensor(const char *name) : Resource(name){}
};

#endif