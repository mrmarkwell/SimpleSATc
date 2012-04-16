##
## Makefile for SimpleSATc
##

EXEC  = SimpleSATc

CC    = gcc

$(EXEC): main.o
	 @echo Linking $(EXEC)
	 @$(CC) main.o -lz -lm -ggdb -Wall -o $@

