CC=gcc
CFLAGS=-Wall

default:
	$(CC) $(CFLAGS) lab0.c -o lab0

#check: default

clean:
	rm -f lab1

.PHONY: clean
