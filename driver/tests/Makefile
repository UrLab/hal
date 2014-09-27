ALL_TESTS_OK: test_HALFS.ok
	touch $@

include ../Makefile.flags

%.ok: %.test
	./$< && touch $@

test_HALFS.test: test_HALFS.c ../HALFS.c
	gcc ${DEFINES} ${CFLAGS} ${LDFLAGS} $^ -o $@

clean:
	rm -f *.ok ALL_TESTS_OK *.test
