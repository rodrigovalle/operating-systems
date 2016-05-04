SHELL=/bin/sh
CC=gcc
CFLAGS=-Wall -Wextra -ggdb -pthread
LDFLAGS=-static
SOURCES=lab2c.c SortedList.c
OBJECTS=$(SOURCES:.c=.o)
TARGET=lab2c

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

test: tests/tests.c SortedList.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

check: test
	./test --color=never

clean:
	rm -f $(TARGET) test *.o

dist: clean
	tar cvzf lab2b-104494120.tar.gz *.c *.h README Makefile *.png

.PHONY: check clean dist check
