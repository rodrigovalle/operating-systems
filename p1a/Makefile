SHELL=/bin/sh
CC=gcc
CFLAGS=-Wall -Wextra -ggdb -pthread
SOURCES=lab1a.c
OBJECTS=$(SOURCES:.c=.o)
TARGET=lab1a

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

check: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o

dist: clean
	tar cvzf lab1a-104494120.tar.gz *.c README Makefile

.PHONY: check clean dist
