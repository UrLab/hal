VERSION = `git rev-parse HEAD`
MODEL=uno
CXXFLAGS=-std=c++11 -pedantic -Wall -Wextra -Wno-unused-parameters

.PHONY: build clean

all: upload.ok driver/driver
build: arduino/.build/uno/firmware.hex

driver/driver:
	make -C driver driver

arduino/.build/uno/firmware.hex: arduino/src/sketch.ino
	cd arduino && ino build -m=$(MODEL) && cd ..

upload.ok: arduino/.build/uno/firmware.hex
	cd arduino && ino upload -m=$(MODEL) && cd .. && touch $@

clean:
	cd arduino && ino clean && cd ..
	rm -f upload.ok
	make -C driver clean

mrproper: clean
	make -C driver mrproper
