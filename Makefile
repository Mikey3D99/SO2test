CC=gcc
CFLAGS=-Wall -g

all: SO2

SO2: main.c
	$(CC) $(CFLAGS) -o SO2 main.c

clean:
	rm -f SO2
