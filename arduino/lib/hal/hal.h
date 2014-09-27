#ifndef DEFINE_HAL_HEADER
#define DEFINE_HAL_HEADER

#include <Arduino.h>

class Resource {
    private:
        const char *_name;
        int _id, _pin;
    public:
        explicit Resource(const char *name, int pin) : _name(name), _id(0), _pin(pin) {}
        const char *name() const {return _name;}
        void setID(int id){_id = id;}
        int id() const {return _id;}
        int pin() const {return _pin;}
};

#define TRIGGER_AFTER 1000

class Trigger : public Resource {
    private:
        const int _active_state; 
        int _count_active, _count_inactive;
    public:
        explicit Trigger(const char *name, int pin, int active_state=HIGH) : 
            Resource(name, pin), _active_state(active_state), _count_active(0), 
            _count_inactive(0)
        {
            pinMode(pin, INPUT);
        }
        bool isActive() const {return _count_active == TRIGGER_AFTER;}
        void check(){hit(digitalRead(pin()));}
        void hit(int state){
            if (state == _active_state && _count_active < TRIGGER_AFTER){
                _count_active ++;
                if (_count_active == TRIGGER_AFTER){
                    Serial.print("!");
                    Serial.print(id(), DEC);
                    Serial.println("1");
                    _count_inactive = 0;
                }
            } else if (state != _active_state && _count_inactive < TRIGGER_AFTER){
                _count_inactive ++;
                if (_count_inactive == TRIGGER_AFTER){
                    Serial.print("!");
                    Serial.print(id(), DEC);
                    Serial.println("0");
                    _count_active = 0;
                }
            }
        }
};

class Switch : public Resource {
    private:
        int _state;
    public:
        explicit Switch(const char *name, int pin) : Resource(name, pin), _state(LOW){pinMode(pin, OUTPUT);}
        void activate(){_state = HIGH;}
        void deactivate(){_state = LOW;}
        bool isActive() const {return _state == HIGH;}
        void writeVal() const {digitalWrite(pin(), _state);}
};

class Animation : public Resource {
    private:
        unsigned char _frames[255];
        unsigned char _len, _i;
        bool _loop, _play;
        unsigned int _delay;
        unsigned long int _tlast;
    public:
        Animation(const char *name, int pin) : 
        Resource(name, pin), _len(0), _loop(true), _play(true), _delay(100){
            memset(_frames, 0, sizeof(_frames));
            pinMode(pin, OUTPUT);
        }
        unsigned char getLen() const {return _len;}
        void setLen(unsigned char len){_len = len;}

        unsigned char getDelay() const {return _delay;}
        void setDelay(unsigned int delay){_delay = delay;}

        bool isLoop() const {return _loop;}
        void setLoop(bool loop){_loop = loop;}

        bool isPlaying() const {return _play;}
        void play(bool stop=false){_play = ! stop;}

        unsigned char &operator[](unsigned char i){return _frames[i];}
        void run(unsigned long int t){
            if (_len == 0 || ! _play){
                analogWrite(pin(), 0);
                return;
            }
            if (t - _tlast >= _delay){
                _i = (_i+1)%_len;
                /* 1 shot: stop when end has been reached */
                if (! _loop && _i == 0)
                    _play = false;
                _tlast = t;
            }
            analogWrite(pin(), _frames[_i]);
        }
};

class Sensor : public Resource {
    public:
        Sensor(const char *name, int pin) : Resource(name, pin){}
        unsigned int getValue() const {return analogRead(pin());}
};

#endif