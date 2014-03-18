#include "Ambianceduino.hpp"
#include <iostream>

using namespace std;

int main(int argc, const char **argv)
{
	Ambianceduino a("/dev/ttyACM0");
	cout << a.version() << endl;	
}