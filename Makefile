CC = gcc

CFLAGS = -g -Wall

all: argus argusd
	test -e fifo || mkfifo fifo
	test -e fifout || mkfifo fifout
	chmod +x script

argus:
	$(CC) $(CFLAGS) argus.c argus.h -o argus

argusd:
	$(CC) $(CFLAGS) argusd.c aux.c aux.h argus.h -o argusd

clean:
	chmod -x script
	rm argus
	rm argusd
	unlink fifo
	unlink fifout
	rm log*