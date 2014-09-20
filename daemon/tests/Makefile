all: test_HALFS.test

%.ok: %.test
	./$< && touch $@

test_HALFS.test: test_HALFS.c ../HALFS.c
	gcc -std=c99 -D_POSIX_C_SOURCE=200809L $^ -o $@
