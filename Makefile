CFLAGS = -g -Wall -std=gnu99 -pedantic

apontamentos.o: apontamentos.c
	gcc $(CFLAGS) apontamentos.c -o apontamentos.o

run: apontamentos.o
	./apontamentos.o

clean:
	rm -f *.o