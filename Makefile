CC=gcc
CFLAGS=-I.
DEPS=lib/%.h
LIB=-lrt

all:
	gcc -g -o shmstore main.c lib/file_parser.c lib/shm.c $(LIB) $(CFLAGS)

clean:
	rm shmstore /dev/shm/shmstore*
