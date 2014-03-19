#include <stdio.h>
#include "arduino-serial-lib.h"
#include <stdbool.h>
#include <errno.h>
#include <string.h>

bool alive = false;

#define strip(s) rstrip(lstrip(s, "\t\r\n"), "\t\r\n")

char *lstrip(char *str, const char *junk)
{
    while (*str != '\0' && strchr(junk, *str))
        str++;
    return str;
}

char *rstrip(char *str, const char *junk)
{
    size_t i = strlen(str);
    while (i > 0 && strchr(junk, str[i-1])){
        i--;
        str[i] = '\0';
    }
    return str;
}

void parseCommand(char *cmd)
{
    bool active;
    size_t len = strlen(cmd);
    if (len == 0)
        return;

    alive = 1;

    printf("PARSE \"%s\"\n", cmd);

    switch (cmd[0]){
        case '-': puts("ON");  break;
        case '_': puts("OFF"); break;
        case '?':
            printf("\033[1mArduino version\033[0m: %s\n", cmd+1);
            break;
        case '@':
            printf("Got analogs: %s\n", cmd+1);
            break;
        case '#':
            printf("FPS correctly set\n");
            break;
        case 'T':
            active = (bool) (cmd[len-1]-'0');
            cmd[len-1] = '\0';
            printf("Trigger %s become %sactive\n", cmd+1, active ? "" : "in");
            break;
        case 'S':
            printf("Switched to state %s\n", cmd+1);
            break;
        case '!':
            printf("Arduino ");
        default:
            printf("error !!!\n");
    }
}

void readCommand(int fd)
{
    char buf[256];
    if (serialport_read_until(fd, buf, '\n', sizeof(buf), 1500))
            parseCommand(strip(buf));
}

int main(int argc, const char **argv)
{
    if (argc == 1){
        fprintf(stderr, "Usage: %s <arduino serial port>\n", argv[0]);
        return 1;
    }

    printf("Opening %s...\n", argv[1]);
    int fd = serialport_init(argv[1], 115200);

    if (fd < 0){
        fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    printf("Starting 1\n");

    while (! alive)
        readCommand(fd);

    serialport_write(fd, "-");
    while (alive)
        readCommand(fd);
}


