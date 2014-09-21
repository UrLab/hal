#include <stdio.h>
#include <unistd.h>
#include "Ambianceduino.h"

HAL arduino;

#define CASEPRINT(n) case n: printf(#n); break
void printState(HALState s)
{
    printf("STATE: ");
    switch (s){
        CASEPRINT(STANDBY);
        CASEPRINT(POWERED);
        CASEPRINT(SHOWTIME);
        CASEPRINT(ALERT);
    }
    puts("");
}

int main(int argc, const char **argv)
{
    HALState state;
    HAL_init(&arduino, "/dev/ttyACM0", 115200);
    printf("Got arduino\n");

    HAL_start(&arduino);

    HAL_READ(&arduino, state, state);
    printState(state);

    HAL_on(&arduino);

    for (unsigned char i=0; i<6; i++){
        HAL_askAnalog(&arduino, i);
        printf("Asked analog%d\n", i);
    }

    if (HAL_lock(&arduino)){
        printState(arduino.state);
        printf("\033[1m      LIGHT\033[0m  Inside: %4d /  Outside: %4d\n", arduino.light_inside, arduino.light_outside);
        printf("\033[1mTEMPERATURE\033[0m Ambiant: %4d / Radiator: %4d\n", arduino.temp_ambiant, arduino.temp_radiator);
        HAL_unlock(&arduino);
    }

    HAL_off(&arduino);
    if (HAL_lock(&arduino)){
        printState(arduino.state);
        HAL_unlock(&arduino);
    }
    HAL_stop(&arduino);

    return 0;
}
