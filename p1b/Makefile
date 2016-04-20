SHELL=/bin/bash
CC=gcc
CFLAGS=-Wall -Wextra -ggdb -pthread -lmcrypt

CLIENTSOURCES=client.c
SERVERSOURCES=server.c
SOURCES=$(CLIENTSOURCES) $(SERVERSOURCES)

CLIENTOBJECTS=$(CLIENTSOURCES:.c=.o)
SERVEROBJECTS=$(SERVERSOURCES:.c=.o)

DIST=lab1b-104494120.tar.gz


all: $(SOURCES) client server

client: $(CLIENTSOURCES) $(CLIENTOBJECTS)
	$(CC) $(CFLAGS) $(CLIENTOBJECTS) -o client

server: $(SERVERSOURCES) $(SERVEROBJECTS)
	$(CC) $(CFLAGS) $(SERVEROBJECTS) -o server

%.o: %.c
	$(CC) -c $< $(CFLAGS)

#check: $(TARGET)
#	./$(TARGET)

clean:
	rm -f *.o server client

dist: clean
	rm -f $(DIST)
	tar cvzf $(DIST) *.c README Makefile

.PHONY: check clean dist
