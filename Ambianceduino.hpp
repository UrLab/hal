#ifndef DEFINE_AMBIANCEDUINO_HEADER
#define DEFINE_AMBIANCEDUINO_HEADER

#include <stdexcept>
#include <string>

class AmbianceduinoNotFound : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

class Ambianceduino {
	private:
		std::string _version, _state;
		int _fd;

	protected:
		bool writeCommand(const char *cmd);
		bool askVersion();
		
	public:
		explicit Ambianceduino(std::string const & dev, int speed=115200);
		~Ambianceduino();
		std::string const & version();
		std::string const & state();

		void readMessage();
};

#endif