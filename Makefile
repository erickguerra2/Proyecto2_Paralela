CC     = gcc
CFLAGS = -Wall -O2
LIBS   = -lcrypto

all: test_des_core

test_des_core: test_des_core.c des_core.c des_core.h
	$(CC) $(CFLAGS) -o $@ test_des_core.c des_core.c $(LIBS)

clean:
	rm -f test_des_core

.PHONY: all clean
