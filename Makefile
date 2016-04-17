SHELL=/bin/sh
CC=gcc
CFLAGS=-Wall -Wextra -ggdb

all: lab0

lab0: lab0.o
	$(CC) lab0.o -o lab0

lab0.o: lab0.c
	$(CC) $(CFLAGS) -c lab0.c

check: lab0
	## create test files (0.5 MB)
	cat /dev/urandom | head -c 1000000  > test.txt
	## checking --input and --output together
	rm -f out.txt
	./lab0 --input=test.txt --output=out.txt
	diff out.txt test.txt
	@echo -e "PASSED\n"
	## reverse the order
	./lab0 --output=out.txt --input=test.txt
	diff out.txt test.txt
	@echo -e "PASSED\n"
	## check --input individually
	./lab0 --input=test.txt | diff - test.txt
	@echo -e "PASSED\n"
	## check --output individually
	cat test.txt | ./lab0 --output=out.txt
	diff test.txt out.txt
	@echo -e "PASSED\n"
	## test --input on a file that doesn't exist; should exit(1) & print to stderr
	rm -f doesntexist.txt
	./lab0 --input=doesntexist.txt; test $$? -eq 1  # check return value
	@echo -e "PASSED\n"
	## test --output on an unopenable file; should exit(2) & print to stderr
	touch canttouchthis.txt
	chmod -rwx canttouchthis.txt
	./lab0 --output=canttouchthis.txt; test $$? -eq 2  # check return value
	@echo -e "PASSED\n"
	## check --segfault
	! ./lab0 --segfault
	@echo -e "PASSED\n"
	## check --segfault --catch; should exit(3) & print to stderr
	./lab0 --segfault --catch > /dev/null; test $$? -eq 3
	@echo
	@echo -e "PASSED\n"
	## flip the order
	./lab0 --catch --segfault > /dev/null; test $$? -eq 3
	@echo
	@echo -e "PASSED\n"
	## remove testfiles if checks succeeded
	rm -f test.txt out.txt canttouchthis.txt
	@echo "======================"
	@echo "|  PASSED ALL TESTS  |"
	@echo "======================"

clean:
	rm -f lab0 *.o test.txt out.txt canttouchthis.txt

dist: clean
	tar cvzf lab0-104494120.tar.gz *

.PHONY: check clean dist
