CC = gcc

CFLAGS = -g -Wall

all: argus servidor
	test -e fifo || mkfifo fifo
	test -e fifout || mkfifo fifout

argus:
	$(CC) $(CFLAGS) main.c -o argus

servidor:
	$(CC) $(CFLAGS) servidor.c aux.c aux.h -o servidor

clean:
	rm argus
	rm servidor
	unlink fifo
	unlink fifout
	rm log*