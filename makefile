##
## Makefile for SimpleSATc
##

EXEC  = SimpleSATc

CC    = gcc

$(EXEC): main.o solver.h
	 @echo Linking $(EXEC)
	 @$(CC) main.o -lz -lm -ggdb -Wall -o $@

