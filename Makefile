VERSION = `git rev-parse HEAD`
ifeq ($(MODEL),)
	MODEL=mega2560
endif

.PHONY: build clean all driver

all: upload.ok driver
build: arduino/.build/uno/firmware.hex

version.h: .git
	@echo "#indef DEFINE_VERSION_HEADER\n#define DEFINE_VERSION_HEADER" > $@
	@echo "#define VERSION \"$$(git log | head -1 | cut -d ' ' -f 2)\"" >> $@
	@echo "#define ARDUINO_VERSION \"$$(git log arduino | head -1 | cut -d ' ' -f 2)\"" >> $@
	@echo "#define DRIVER_VERSION \"$$(git log driver | head -1 | cut -d ' ' -f 2)\"" >> $@
	@echo "#endif" >> $@

driver:
	+make -C driver

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
