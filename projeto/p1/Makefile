# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../ -lpthread
LDFLAGS=-lm

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs-nosync tecnicofs-mutex #tecnicofs-rwlock


tecnicofs-nosync: lib/bst.o fs.o main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-nosync lib/bst.o fs.o main.o

tecnicofs-mutex: lib/bst.o fs.o main-mu.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-mutex lib/bst.o fs.o main-mu.o



lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) -o lib/bst.o -c lib/bst.c

fs.o: fs.c fs.h lib/bst.h
	$(CC) $(CFLAGS) -o fs.o -c fs.c



main.o: main.c fs.h lib/bst.h
	$(CC) $(CFLAGS) -o main.o -c main.c

main-mu.o:
	$(CC) $(CFLAGS) -o main-mu.o -c main.c -DMUTEX


clean:
	@echo Cleaning...
	rm -f lib/*.o *.o tecnicofs
	##remover antes da entrega{
		rm -f *.out core output
	#}

run: tecnicofs-mutex
	./tecnicofs inputs/test4.txt myout.out 3
