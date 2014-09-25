VERSION = `git rev-parse HEAD`
MODEL=mega2560
CXXFLAGS=-std=c++11 -pedantic -Wall -Wextra -Wno-unused-parameters

.PHONY: build clean all

all: upload.ok driver/driver driver/tests/ALL_TESTS_OK
build: arduino/.build/uno/firmware.hex

driver/driver:
	make -C driver driver

driver/tests/ALL_TESTS_OK: driver/
	make -C driver/tests

arduino/.build/uno/firmware.hex: arduino/src/sketch.ino
	cd arduino && ino build -m=$(MODEL) && cd ..

upload.ok: arduino/.build/uno/firmware.hex
	cd arduino && ino upload -m=$(MODEL) && cd .. && touch $@

clean:
	cd arduino && ino clean && cd ..
	rm -f upload.ok
	make -C driver clean
	make -C driver/tests clean

mrproper: clean
	make -C driver mrproper
