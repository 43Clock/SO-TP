CC = gcc

CFLAGS = -g -Wall

all: argus argusd
	test -e fifo || mkfifo fifo
	test -e fifout || mkfifo fifout

argus:
	$(CC) $(CFLAGS) argus.c -o argus

argusd:
	$(CC) $(CFLAGS) argusd.c aux.c aux.h -o argusd

clean:
	rm argus
	rm argusd
	unlink fifo
	unlink fifout
	rm log*