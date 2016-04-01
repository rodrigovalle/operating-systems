CC=gcc
CFLAGS=-Wall

default:
	$(CC) $(CFLAGS) lab0.c -o lab0

check: default
	echo "smoke test" > test.txt
	# checking --input and --output together
	./lab0 --input=test.txt --output=out.txt
	diff out.txt test.txt
	# reverse the order
	./lab0 --output=out.txt --input=test.txt
	diff out.txt test.txt
	# test --input on files that don't exist
	rm -f doesntexist.txt
	[[ !$$(./lab0 --input=doesntexist.txt) ]]
	# test --output on unopenable file
	touch canttouchthis.txt
	chmod -rwx canttouchthis.txt
	[[ !$$(./lab0 --output=canttouchthis.txt) ]]
	# check --input individually
	./lab0 --input=test.txt | diff - test.txt
	# check --output individually
	cat test.txt | ./lab0 --output=out.txt
	diff test.txt out.txt
	# check --segfault
	!(./lab0 --segfault)
	# check --catch --segfault
	./lab0 --catch --segfault 2>&1 1>/dev/null | grep "error: segfault caught"
	# check --segfault --catch
	./lab0 --segfault --catch 2>&1 1>/dev/null | grep "error: segfault caught"

clean:
	rm -f lab0 test.txt out.txt lab0-104494120.tar.gz canttouchthis.txt

dist: clean
	tar cvzf lab0-104494120.tar.gz *

.PHONY: check clean dist
