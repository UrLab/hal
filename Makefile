VERSION = `git rev-parse HEAD`
MODEL=uno
CXXFLAGS=-std=c++11 -pedantic -Wall -Wextra -Wno-unused-parameters	

.PHONY: build clean

all: upload daemon
build: arduino/.build/uno/firmware.hex

daemon: test.cpp Ambianceduino.cpp
	${CXX} ${CXXFLAGS} -o $@ $^

arduino/.build/uno/firmware.hex: arduino/src/sketch.ino
	cd arduino && ino build -m=$(MODEL)

arduino/src/sketch.ino: arduino/src/sketch.ino.tpl .git
	sed -e "s/{{version}}/${VERSION}/" < $< > $@

upload: arduino/.build/uno/firmware.hex
	cd arduino && ino upload -m=$(MODEL)

clean:
	rm -f arduino/src/sketch.ino
	cd arduino && ino clean
