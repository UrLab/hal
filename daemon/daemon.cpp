#include "Ambianceduino.hpp"
#include <iostream>
#include <unistd.h>

using namespace std;

int main(int argc, const char **argv)
{
	if (argc < 2){
		cerr << "USAGE: " << argv[0] << " <arduino serial port>" << endl;
		return EXIT_FAILURE;
	}
	cout << "Starting..." << endl;
	Ambianceduino a(argv[1]);
	
	cout << "Getting version..." << endl;

	cout << a.version() << endl;

	return EXIT_FAILURE;
}