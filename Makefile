CC     = gcc
CFLAGS = -Wall -O2
LIBS   = -lcrypto

all: test_des_core cifrar

test_des_core: test_des_core.c des_core.c des_core.h
	$(CC) $(CFLAGS) -o $@ test_des_core.c des_core.c $(LIBS)

cifrar: cifrar.c des_core.c des_core.h
	$(CC) $(CFLAGS) -o $@ cifrar.c des_core.c $(LIBS)

clean:
	rm -f test_des_core cifrar

.PHONY: all clean
