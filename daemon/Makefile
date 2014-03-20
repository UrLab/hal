CC = gcc
CFLAGS = -std=c99 -pedantic -Wno-unused-parameter -Wall -Wextra
LDFLAGS = -lpthread

test: test.o arduino-serial-lib.o Ambianceduino.o
	${CC} ${LDFLAGS} -o $@ $^

driver: driver.o arduino-serial-lib.o Ambianceduino.o
	${CC} ${LDFLAGS} `pkg-config --libs fuse` -o $@ $^

driver.o: driver.c
	${CC} ${CFLAGS} ${CPPFLAGS} `pkg-config --cflags fuse` -c -o $@ $<

%.o: %.c
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<

.PHONY: clean
clean:
	rm -f *.o
