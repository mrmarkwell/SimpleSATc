##
## Makefile for SimpleSATc
##

CSRCS = $(wildcard *.c)
COBJS = $(addsuffix .o, $(basename $(CSRCS)))

EXEC  = SimpleSATc
CC    = gcc

$(EXEC): $(COBJS)
	 @echo Linking $(EXEC)
	 @$(CC) $(COBJS) -lz -lm -ggdb -Wall -o $@

clean:
	 @rm -f $(EXEC) $(COBJS)
