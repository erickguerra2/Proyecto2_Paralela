CC     = gcc
CFLAGS = -Wall -O2
LIBS   = -lcrypto
MPICC  = mpicc

all: test_des_core cifrar secuencial bruteforce_mpi

test_des_core: test_des_core.c des_core.c des_core.h
	$(CC) $(CFLAGS) -o $@ test_des_core.c des_core.c $(LIBS)

cifrar: cifrar.c des_core.c des_core.h
	$(CC) $(CFLAGS) -o $@ cifrar.c des_core.c $(LIBS)

secuencial: secuencial.c des_core.c des_core.h
	$(CC) $(CFLAGS) -o $@ secuencial.c des_core.c $(LIBS)

bruteforce_mpi: bruteforce_mpi.c distribucion.c distribucion.h des_core.c des_core.h
	$(MPICC) $(CFLAGS) -o $@ bruteforce_mpi.c distribucion.c des_core.c $(LIBS)

clean:
	rm -f test_des_core cifrar secuencial bruteforce_mpi

.PHONY: all clean
