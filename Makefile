CC = gcc
CXXFLAGS = -O2 -Wall
ASM = nasm
ASMFLAGS = -f elf64
LD = ld

all: strings.o proc.o scan.o lema.o lema

lema: lema.o scan.o proc.o
	$(CC) $(CXXFLAGS) -o lema.exe lema.o scan.o proc.o strings.o

lema.o: lema.c scan.h proc.h lema.h
	$(CC) $(CXXFLAGS) -c -O2 -o $@ $<

scan.o: scan.c scan.h
	$(CC) $(CXXFLAGS) -c -O2 -o $@ $<

strings.o: strings.c strings.h
	$(CC) $(CXXFLAGS) -c -O2 -o $@ $<

proc.o: proc.c proc.h scan.h
	$(CC) $(CXXFLAGS) -c -O2 -o $@ $<

print: print.asm
	$(ASM) $(ASMFLAGS) print.asm
	$(LD) print.o -o print.exe

clean:
	rm -f *.exe *.o
