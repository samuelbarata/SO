CFLAGS = -g -Wall -std=gnu99 -pedantic
# -pedantic
# -g # adds debug info; required for using gdb or valgrind

test: test.o bst.o
	gcc $(CFLAGS) test.o bst.o -o test

test.o: test.c bst.h
	gcc $(CFLAGS) -c test.c -o test.o

bst.o: bst.c bst.h
	gcc $(CFLAGS) -c bst.c -o bst.o

clean:
	rm -f *.o test core

clean2:
	rm -f *.o

run: test
	./test

run1: test
	./test < tree1.txt

run2: test
	./test < tree2.txt

run3: test
	echo 'a 20 a 15 a 30 a 22 a 10 a 29 p l' | ./test > /dev/null

run4: test
	valgrind --tool=memcheck --leak-check=full ./test < tree1.txt