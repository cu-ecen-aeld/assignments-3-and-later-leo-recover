CC=$(CROSS_COMPILE)gcc

CFLAGS+=-Wall -Werror -g

all: aesdsocket

aesdsocket : aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o

aesdsocket.o : aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c

clean : 
	-rm aesdsocket aesdsocket.o