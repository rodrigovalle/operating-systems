# easiest makefile ever
# TODO: change this, final solution will run in same directory as all csv files
run:
	./lab3b
check: run
	diff ./solutions/lab3b_check.txt lab3_check.txt
clean:

dist:
	tar cvzf lab3b-104494120.tar.gz lab3b README Makefile
.PHONY: run check clean dist
