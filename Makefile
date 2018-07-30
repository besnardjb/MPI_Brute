CC=mpicc
OPT=-O3 -march=native
LDFLAG=-lcrypto -lssl

all: rainbow libmpibrute.so

rainbow: mpi_brute.c rainbow.c
	$(CC) $(OPT) $(LDFLAG) $^ -o $@

libmpibrute.so: mpi_brute.c
	$(CC) $(OPT) --shared -fpic $^ -o $@

clean:
	rm -fr rainbow libmpibrute.so
