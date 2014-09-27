include Makefile.flags

TARGET = driver
OBJS = HALFS.o arduino-serial-lib.o com.o

all: ${TARGET}

${TARGET}: ${TARGET}.o ${OBJS}
	${CC} -o $@ $^ ${LDFLAGS}

%.o: %.c
	${CC} ${DEFINES} ${WARNINGS} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<

.PHONY: clean mrproper tests
clean:
	rm -f *.o
	+make -C tests clean

mrproper:
	rm -f ${TARGET}

tests:
	+make -C $@
