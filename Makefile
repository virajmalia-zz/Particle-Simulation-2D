#
# Bridges - PSC
#
# Intel Compilers are loaded by default; for other compilers please check the module list
#
CC = icpc
#CC = g++
MPCC = mpicxx -cxx=icpc
OPENMP = -qopenmp
#CFLAGS = -g -O3 -xHOST -Ofast -march=core-avx2 -funroll-loops -march=core-avx2 -vec -std=c++11
CFLAGS = -g -std=c++11
LIBS =


TARGETS = serial openmp mpi autograder

all:	$(TARGETS)

serial: serial.o common.o
	$(CC) -o $@ $(LIBS) serial.o common.o
autograder: autograder.o common.o
	$(CC) -o $@ $(LIBS) autograder.o common.o
openmp: openmp.o common.o
	$(CC) -o $@ $(LIBS) $(OPENMP) openmp.o common.o
mpi: mpi.o common.o 
	$(MPCC) -o $@ $(LIBS) $(MPILIBS) mpi.o common.o 

autograder.o: autograder.cpp common.h
	$(CC) -c $(CFLAGS) autograder.cpp
openmp.o: openmp.cpp common.h
	$(CC) -c $(OPENMP) $(CFLAGS) openmp.cpp
serial.o: serial.cpp common.h
	$(CC) -c $(CFLAGS) serial.cpp
mpi.o: mpi.cpp common.h 
	$(MPCC) -c $(CFLAGS) mpi.cpp
common.o: common.cpp common.h
	$(CC) -c $(CFLAGS) common.cpp
# mpi_tools.o: mpi_tools.cpp mpi_tools.h
# 	$(MPCC) -c $(CFLAGS) mpi_tools.cpp

clean:
	rm -rf *.o $(TARGETS) *.stdout *.txt *.optrpt out
