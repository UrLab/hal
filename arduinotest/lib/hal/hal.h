#ifndef DEFINE_HAL_HEADER
#define DEFINE_HAL_HEADER

#include <Arduino.h>

class Resource {
    private:
        const char *_name;
        const int _id;
    public:
        explicit Resource(const char *name, int id) : _name(name), _id(id) {}
        const char *name() {return _name;}
        const int id() {return _id;}
};

#define TRIGGER_AFTER 10

class Trigger : public Resource {
    private:
        const int _active_state, _pin; 
        int _count_state;
    public:
        explicit Trigger(const char *name, int id, int pin, int active_state=HIGH) : 
            Resource(name, id), _active_state(active_state), _pin(pin), _count_state(0)
        {
            pinMode(pin, INPUT);
        }
        bool isActive() const {return _count_state == TRIGGER_AFTER;}
        void check(){hit(digitalRead(_pin));}
        void hit(int state){
            if (state == _active_state && _count_state < TRIGGER_AFTER){
                _count_state ++;
                if (_count_state == TRIGGER_AFTER){
                    Serial.print("!");
                    Serial.print(id(), DEC);
                    Serial.println("1");
                }
            } else if (state != _active_state){
                if (_count_state == TRIGGER_AFTER){
                    Serial.print("!");
                    Serial.print(id(), DEC);
                    Serial.println("0");
                }
                _count_state = 0;
            }
        }
};

class Switch : public Resource {
    private:
        int _state, _pin;
    public:
        explicit Switch(const char *name, int id, int pin) : Resource(name, id), _state(LOW), _pin(pin){}
        void activate(){_state = HIGH;}
        void deactivate(){_state = LOW;}
        bool isActive() const {return _state == HIGH;}
        void writeVal() const {digitalWrite(_pin, _state);}
};

class Animation : public Resource {
    public:
        Animation(const char *name, int id) : Resource(name, id){}
};

class Sensor : public Resource {
    public:
        Sensor(const char *name, int id) : Resource(name, id){}
};

#endif