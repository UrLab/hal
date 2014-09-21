all: test_HALFS.ok

include ../Makefile

%.ok: %.test
	./$< && touch $@

test_HALFS.test: test_HALFS.c ../HALFS.c
	gcc ${DEFINES} ${CFLAGS} ${LDFLAGS} $^ -o $@
