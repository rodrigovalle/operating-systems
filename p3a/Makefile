SHELL=/bin/sh
CC=gcc
CFLAGS=-Wall -Wextra -ggdb -pthread
LDFLAGS=-static
SOURCES=lab3a.c
OBJECTS=$(SOURCES:.c=.o)
TARGET=lab3a

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

clobber: clean
	rm -f *.csv

dist: clean
	tar cvzf lab3a-104494120.tar.gz *.c *.h README Makefile *.png

.PHONY: check clean clobber dist check
