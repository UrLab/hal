#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <stdbool.h>

int set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0){
        fprintf(stderr, "error %d in tcgetattr: %s", errno, strerror(errno));
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // ignore break signal
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    //tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0){
        fprintf(stderr, "error %d from tcsetattr: %s", errno, strerror(errno));
        return -1;
    }
    return 0;
}

void set_blocking (int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0){
        fprintf(stderr, "error %d from tggetattr: %s", errno, strerror(errno));
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        fprintf(stderr, "error %d in tcsetattr: %s", errno, strerror(errno));
}

struct {
    bool booted;
    bool powered;
    char status[32];
    char version[41];
    int fd;
} arduino = {false, false, "", ""};

void parseCommand(char *cmd, size_t len)
{
    arduino.booted = true;
    bool active;

    printf("PARSE \"%s\"\n", cmd);

    switch (cmd[0]){
        case '-': arduino.powered = true; break;
        case '_': arduino.powered = false; break;
        case '?':
            printf("\033[1mArduino version\033[0m: %s\n", cmd+1);
            strcpy(arduino.version, cmd+1);
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
            strcpy(arduino.status, cmd+1);
            break;
        case '!':
            printf("Arduino ");
        default:
            printf("error !!!");
    }
}

void readArduino(int fd)
{
    static char buf[128] = {'\0'};
    size_t r = 0;

    while (r<sizeof(buf) && read(fd, buf+r, 1) == 1){
        if (strchr("\n\r", buf[r])){
            buf[r] = '\0';
            break;
        }
        r++;
    }

    if (r > 0)
        parseCommand(buf, r);
}

bool writeArduino(int fd, const char *cmd)
{
    size_t l = strlen(cmd);
    int r = write(fd, cmd, l);
    return r == (int) l;
}

bool askAnalogs(int fd)
{
    return writeArduino(fd, "@");
}

bool setFPS(int fd, unsigned char anim_id, unsigned char fps)
{
    char buf[4] = "#xx";
    buf[1] = anim_id;
    buf[2] = fps;
    return writeArduino(fd, buf);
}

bool setOn(int fd)
{
    return writeArduino(fd, "-");
}

bool setOff(int fd)
{
    return writeArduino(fd, "_");
}

int main(int argc, const char **argv)
{
    int fd = open("/dev/ttyACM0", O_RDWR);
    set_interface_attribs(fd, 115200, 0);
    set_blocking(fd, 1);

    while (! arduino.booted)
        readArduino(fd);

    askAnalogs(fd);
    setOn(fd);

    while (true){
        readArduino(fd);
    }
}