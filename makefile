# Makefile for siren software
CC = gcc

BINDIR = /usr/local/bin/

SRC = src

INCLUDE = -I$(SRC)
OPTIONS = -DTRACE -DENABDS
LDLIBS  = -lm

CFLAGS = -O3 -pedantic -Wall -Wno-unused-but-set-variable $(INCLUDE) $(OPTIONS) -static -s

all: siren

siren: siren.o channels.o code.o ephemeris.o errors.o io.o signal.o options.o toml.o utils.o

siren.o: $(SRC)/siren.c
	$(CC) -c $(CFLAGS) $(SRC)/siren.c
channels.o: $(SRC)/channels.c
	$(CC) -c $(CFLAGS) $(SRC)/channels.c
code.o: $(SRC)/code.c
	$(CC) -c $(CFLAGS) $(SRC)/code.c
ephemeris.o: $(SRC)/ephemeris.c
	$(CC) -c $(CFLAGS) $(SRC)/ephemeris.c
errors.o: $(SRC)/errors.c
	$(CC) -c $(CFLAGS) $(SRC)/errors.c
io.o: $(SRC)/io.c
	$(CC) -c $(CFLAGS) $(SRC)/io.c
options.o: $(SRC)/options.c
	$(CC) -c $(CFLAGS) $(SRC)/options.c
signal.o: $(SRC)/signal.c
	$(CC) -c $(CFLAGS) $(SRC)/signal.c
toml.o: $(SRC)/toml.c
	$(CC) -c $(CFLAGS) $(SRC)/toml.c
utils.o: $(SRC)/utils.c
	$(CC) -c $(CFLAGS) $(SRC)/utils.c

install:
	cp siren $(BINDIR)

clean:
	rm -f siren siren.exe *.o
