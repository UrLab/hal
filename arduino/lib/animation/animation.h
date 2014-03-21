#ifndef DEFINE_ANIMATION_HEADER
#define DEFINE_ANIMATION_HEADER

#include <Arduino.h>

/*! Number type */
typedef unsigned char Unit;

class Animation {
    private:
        unsigned int _len;
        Unit _curve[256];
        bool _default_mode, _on;
        Unit _frame;
        unsigned int _delay;
        long unsigned int _last_tick;

    protected:
        virtual void output(Unit val){}

    public:
        explicit Animation(unsigned int fps=25);
        void setFPS(unsigned int fps);
        void play(unsigned long int tick=millis());
        /*! Reset to default curve */
        void reset();

        virtual void on();
        virtual void off();

        /*! Access custom curve */
        Unit & operator[](Unit i);
        Unit const & operator[](Unit i) const;
        /*! Set custom animation length, and switch to custom mode */
        void setLength(unsigned int length);
};

class Ledstrip : public Animation {
    private:
        int _pin;
    protected:
        void output(Unit val);
    public:
        explicit Ledstrip(int pin);
        void off();
};

#endif
