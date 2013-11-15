VERSION = `git log | head -1 | cut -d ' ' -f 2`

.PHONY: upload build clean

all: version.py upload
build: arduino/.build/uno/firmware.hex

arduino/src/sketch.ino: arduino/src/sketch.ino.tpl .git
	sed -e "s/{{version}}/${VERSION}/" < $< > $@

version.py: .git
	echo FIRMWARE_VERSION = \"${VERSION}\" > version.py

upload: arduino/.build/uno/firmware.hex
	cd arduino && ino upload
	
arduino/.build/uno/firmware.hex: arduino/src/sketch.ino
	cd arduino && ino build

clean:
	rm -f arduino/src/sketch.ino version.py
	cd arduino
	ino clean
	cd -
