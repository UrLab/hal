CC = gcc
CFLAGS = -std=c99
CXXFLAGS = -std=c++11
CPPFLAGS = -pedantic -Wall -Wextra

daemon: daemon.o arduino-serial-lib.o Ambianceduino.o
	${CXX} -o $@ $^

driver: driver.o arduino-serial-lib.o Ambianceduino.o
	${CXX} `pkg-config --libs fuse` -o $@ $^

driver.o: driver.cpp
	${CXX} ${CXXFLAGS} ${CPPFLAGS} `pkg-config --cflags fuse` -c -o $@ $<

%.o: %.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<

%.o: %.cpp
	${CXX} ${CXXFLAGS} ${CPPFLAGS} -c -o $@ $<

.PHONY: clean
clean:
	rm -f *.o
