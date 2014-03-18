#include "Ambianceduino.hpp"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <termios.h>
#include <errno.h>

#include <cstring>
#include <cstdlib>

Ambianceduino::Ambianceduino(std::string const & dev, int speed) : 
	_fd(open(dev.c_str(), O_RDRW))
{
	if (_fd < 0)
		throw AmbianceduinoNotFound(dev);

	struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr (_fd, &tty) != 0){
        fprintf(stderr, "error %d in tcgetattr: %s", errno, strerror(errno));
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
    tty.c_cflag &= ~CSTOPB;

    if (tcsetattr (_fd, TCSANOW, &tty) != 0){
        fprintf(stderr, "error %d from tcsetattr: %s", errno, strerror(errno));
    }
}

Ambianceduino::~Ambianceduino()
{
	if (_fd > 0)
		close(_fd);
}

bool Ambianceduino::writeCommand(const char *cmd)
{
	int l = strlen(cmd);
	return write(_fd, cmd, l) == l;
}

bool Ambianceduino::askVersion()
{
	return writeCommand("?");
}

std::string const & Ambianceduino::version()
{
	askVersion();
	while (_version.length() == 0)
		readMessage();
	return _version;
}

std::string const & Ambianceduino::state()
{
	return _state;
}

void Ambianceduino::readMessage()
{
	char buf[1024];
	int r = 0;
	while (r<sizeof(buf) && read(_fd, buf+r, 1) == 1){
		if (buf[r] == '\r')
			buf[r] = '\0';
		else if (buf[r] == '\n'){
			buf[r] = '\0';
			break;
		} else {
			r++;
		}
	}

	switch (buf[0]){
		case '?':
			_version = std::string(buf+1);
			break;
	}
}