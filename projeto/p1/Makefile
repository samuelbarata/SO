# Makefile, versao 1.1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../
LDFLAGS=-lm -lpthread

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs-nosync tecnicofs-mutex tecnicofs-rwlock

debug: CFLAGS += -g
debug: all

tecnicofs-nosync: lib/bst.o fs.o main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-nosync lib/bst.o fs.o main.o

tecnicofs-mutex: lib/bst.o fs.o main-mu.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-mutex lib/bst.o fs.o main-mu.o

tecnicofs-rwlock: lib/bst.o fs.o main-rw.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-rwlock lib/bst.o fs.o main-rw.o



lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) -o lib/bst.o -c lib/bst.c

fs.o: fs.c fs.h lib/bst.h
	$(CC) $(CFLAGS) -o fs.o -c fs.c



main.o: main.c fs.h lib/bst.h
	$(CC) $(CFLAGS) -o main.o -c main.c

main-mu.o:
	$(CC) $(CFLAGS) -o main-mu.o -c main.c -DMUTEX

main-rw.o:
	$(CC) $(CFLAGS) -o main-rw.o -c main.c -DRWLOCK


clean:
	@echo Cleaning...
	@rm -f lib/*.o *.o tecnicofs*
	@rm -f *.out core inputs/*.out
	@rm -f -r output
	@clear

run: all
	@./tester.sh tecnicofs-nosync 0 1
	@./tester.sh tecnicofs-mutex 1 100
	@./tester.sh tecnicofs-mutex 2 100
	@./tester.sh tecnicofs-mutex 4 100
	@./tester.sh tecnicofs-mutex 8 100
	@./tester.sh tecnicofs-mutex 16 500
	@./tester.sh tecnicofs-mutex 32 500
	@./tester.sh tecnicofs-rwlock 1 100
	@./tester.sh tecnicofs-rwlock 2 100
	@./tester.sh tecnicofs-rwlock 4 100
	@./tester.sh tecnicofs-rwlock 8 100
	@./tester.sh tecnicofs-rwlock 16 500
	@./tester.sh tecnicofs-rwlock 32 500
