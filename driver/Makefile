TARGET = driver
OBJS = HALFS.o arduino-serial-lib.o com.o

CC = gcc
DEFINES = -DFUSE_USE_VERSION=26 -D__USE_BSD -D_POSIX_C_SOURCE=200809L
WARNINGS = -Wno-unused-function -Wno-unused-parameter -Wall -Wextra -Werror
CFLAGS = -std=c99 -pedantic $(shell pkg-config --cflags fuse)
LDFLAGS = -lpthread $(shell pkg-config --libs fuse)

all: ${TARGET}

${TARGET}: ${TARGET}.o ${OBJS}
	${CC} -o $@ $^ ${LDFLAGS}

%.o: %.c
	${CC} ${DEFINES} ${WARNINGS} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<

.PHONY: clean mrproper
clean:
	rm -f *.o

mrproper:
	rm -f ${TARGET}
