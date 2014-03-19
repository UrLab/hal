#ifndef DEFINE_AMBIANCEDUINO_HEADER
#define DEFINE_AMBIANCEDUINO_HEADER

#include <stdexcept>
#include <string>
#include <functional>
#include <unordered_map>
#include <pthread.h>

typedef std::unordered_map<std::string, bool> TrigMap;

class IOError : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

class AmbianceduinoNotFound : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

class Ambianceduino {
	private:
		std::string _version, _state;
		int _fd;
		TrigMap _triggers;

	protected:
		void askVersion();
		void parseTrigger(char *trigger);
		
	public:
		explicit Ambianceduino(std::string const & dev, int speed=115200, int boot_secs=2);
		~Ambianceduino();
		std::string const & version();
		std::string const & state();

		void on();
		void off();

		void readMessage();
};

#endif