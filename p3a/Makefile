SHELL=/bin/sh
CC=gcc
CFLAGS=-Wall -Wextra -ggdb -Wno-missing-field-initializers
LDFLAGS=-static
SOURCES=lab3a.c
OBJECTS=$(SOURCES:.c=.o)
TARGET=lab3a

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

check: test
	./test --color=never

clean:
	rm -f $(TARGET) test *.o

clobber: clean
	rm -f *.csv

dist: clean
	tar cvzf lab3a-104494120.tar.gz *.c README Makefile

.PHONY: check clean clobber dist check
