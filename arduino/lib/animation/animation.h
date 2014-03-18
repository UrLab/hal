#ifndef DEFINE_ANIMATION_HEADER
#define DEFINE_ANIMATION_HEADER

#ifdef FAKE_ARDUINO
unsigned long int millis(){
    static unsigned long int tick;
    tick++;
    return tick;
}

#define analogWrite(x, y)
#define digitalWrite(x, y)
#define tone(x, y)
#endif

/*! Number type */
typedef unsigned char Unit;

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

/*! A vector of storage size L */
template <const Unit L=255>
class Animation {
    private:
        /*! Animation frames */
        Unit _curve[L];
        /*! Animation size, in Units */
        Unit _len;
    public:
        Animation() : _len(L){
            for (Unit i=0; i<L; i++)
                _curve[i] = 0;
        }
        virtual ~Animation(){}

        void resetDefault(){
            setLen(L);
            Unit r = (sizeof(predefined_sin)/sizeof(Unit))/L;
            for (Unit i=0; i<L; i++)
                (*this)[i] = predefined_sin[i*r];
        }

        /*! access ith frame */
        Unit & operator[](Unit const i){return _curve[i%_len];}
        Unit const & operator[](Unit const i) const {return _curve[i%_len];}

        /*! Set active region to the first length bytes of Animation frames */
        void setLen(Unit length){
            if (length <= L)
                _len = length;
            else
                _len = L;
        }
        /*! Return the active region length */
        Unit len() const {return _len;}
};

/*! Common interface for sound/light/... output */
class Playable {
    private:
        /*! Last time the state changed, in ms */
        unsigned long int _last_tick;
        /*! Current state */
        Unit _current_value, _current_delay;
        /* Should be playing now ? */
        bool _active;
        /*! switch to next frame */
        virtual void next() = 0;
        /*! Return value for this frame */
        virtual Unit getValue() const = 0;
        /*! Return delay for this frame */
        virtual Unit getDelay() const = 0;
        /*! Do something with current value */
        virtual void output(Unit val){}

    public:
        Playable() : 
            _last_tick(0), _current_value(0), _current_delay(0), _active(true) 
        {output(0);}

        /*! Activate output */
        void  on(){_active = true;}
        /*! Deactivate output */
        void off(){_active = false; output(0);}

        /*! main method to use in loops */
        void play(){
            if (! _active) 
                return;
            unsigned long now = millis();
            if (now-_last_tick > _current_delay){
                next();
                _current_value = getValue();
                _current_delay = getDelay();
                _last_tick = now;
            }
            output(_current_value);
        }
};

/*! A playable animation with constant FPS */
template <const Unit L=255>
class AnimationPlayer : public Animation<L>, public Playable {
    private:
        Unit _frame, _delay;
        virtual void next(){_frame = (_frame+1)%Animation<L>::len();}
        virtual Unit getValue() const {return Animation<L>::operator[](_frame);}
        virtual Unit getDelay() const {return _delay;}
    public:
        AnimationPlayer(Unit fps=25) : 
            Animation<L>(), Playable(), _frame(0), 
            _delay((fps > 0) ? 1000/fps : 1000)
        {}
        void setFPS(Unit fps){_delay = 1000/fps;}
};

#define max(a, b) ((a)>(b)) ? (a) : (b)

/*! A playable animation, where each frame has its own duration */
template <const Unit L=255>
class PartitionPlayer : public Animation<L>, public Playable {
    private:
        Unit _frame;
        Animation<L> _durations;
        virtual void next(){
            _frame = (_frame+1) % max(Animation<L>::len(), _durations.len());
        }
        virtual Unit getValue() const {return Animation<L>::operator[](_frame);}
        virtual Unit getDelay() const {return _durations[_frame];}
    public:
        PartitionPlayer() : Animation<L>(), Playable(), _frame(0) {}
        /*! Return duration for the ith frame */
        Unit & duration(Unit i){return _durations[i];}
        Unit const & duration(Unit i) const {return _durations[i];}
};

/*! A PWM controlable ledstrip with constant delay */
template <const Unit L=255>
class DCLedstrip : public AnimationPlayer<L> {
    private:
        int _pin;
        void output(Unit val){analogWrite(_pin, val);}
    public:
        DCLedstrip(int pin, Unit fps=25) : AnimationPlayer<L>(fps), _pin(pin){
            pinMode(_pin, OUTPUT);
        }
};

/*! A on/off ledstrip (use on() and off()) */
class ACLedstrip : public AnimationPlayer<1> {
    private:
        int _pin;
        void output(Unit val){digitalWrite(_pin, val);}
    public:
        ACLedstrip(int pin) : AnimationPlayer<1>(1), _pin(pin){
            (*this)[0] = 0xff;
            pinMode(pin, OUTPUT);
        }
};

template <const Unit L=255>
class Buzzer : public PartitionPlayer<L> {
    private:
        int _pin;
        void output(Unit val){tone(_pin, val*4);}
    public:
        Buzzer(int pin) : _pin(pin){
            pinMode(_pin, OUTPUT);
        }
};

#endif
