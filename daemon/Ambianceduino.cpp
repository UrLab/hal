#include "Ambianceduino.hpp"
#include "arduino-serial-lib.h"

#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>

#include <iostream>

using namespace std;

#define STRIP_JUNK "\r\n\t"
#define strip(s) rstrip(lstrip(s, STRIP_JUNK), STRIP_JUNK)

static inline char *lstrip(char *str, const char *junk)
{
    while (*str != '\0' && strchr(junk, *str))
        str++;
    return str;
}

static inline char *rstrip(char *str, const char *junk)
{
    size_t i = strlen(str);
    while (i > 0 && strchr(junk, str[i-1])){
        i--;
        str[i] = '\0';
    }
    return str;
}

Ambianceduino::Ambianceduino(std::string const & dev, int speed, int boot_secs)
{
	_fd = serialport_init(dev.c_str(), speed);
	if (_fd < 0)
		throw IOError(std::string("Unable to open ")+dev+": "+strerror(errno));
	sleep(boot_secs);
}

Ambianceduino::~Ambianceduino()
{
	if (_fd > 0)
		serialport_close(_fd);
}

void Ambianceduino::askVersion()
{
	serialport_write(_fd, "?");
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

void Ambianceduino::parseTrigger(char *cmd)
{
	size_t l = strlen(cmd);
	bool active = cmd[l-1] == '1';
	cmd[l-1] = '\0';

	_triggers[cmd+1] = active;
	
	if (active)
		cout << "Trigger " << cmd+1 << " on " << endl;
	else
		cout << "Trigger " << cmd+1 << " off " << endl;
}

void Ambianceduino::readMessage()
{
	char buf[256];
	if (serialport_read_until(_fd, buf, '\n', sizeof(buf), 1500)){
		char *command = strip(buf);

		//cout << "PARSE \"" << command << "\"" << endl;
		switch (command[0]){
			case '-': cout << " ON" << endl; break;
			case '_': cout << "OFF" << endl; break;
			case '?':
				_version = std::string(command+1);
				break;
			case 'S':
				_state = std::string(command+1);
				break;
			case 'T':
				parseTrigger(command);
				break;
		}
	}
}

void Ambianceduino::on()
{
	serialport_write(_fd, "-");
}

void Ambianceduino::off()
{
	serialport_write(_fd, "_");
}
