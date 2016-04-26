CC=g++
CFLAGS=-std=c++11
PKGCONFIG=`pkg-config --libs ncurses`

all: nes

nes: nes.cpp cpu.cpp cpu.h instruction.cpp instruction.h 
	$(CC) $(CFLAGS) -o nes nes.cpp cpu.cpp instruction.cpp $(PKGCONFIG) -pthread

clean:
	rm -f *.o nes
