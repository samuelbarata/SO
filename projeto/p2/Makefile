# Makefile, versao 2.1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

SOURCES = main.c fs.c sync.c
SOURCES+= lib/bst.c lib/hash.c
OBJS_NOSYNC = $(SOURCES:%.c=%.o)
OBJS_MUTEX  = $(SOURCES:%.c=%-mutex.o)
OBJS_RWLOCK = $(SOURCES:%.c=%-rwlock.o)
OBJS = $(OBJS_NOSYNC) $(OBJS_MUTEX) $(OBJS_RWLOCK)
CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../ -g -DDEBUGG
LDFLAGS=-lm -pthread
TARGETS = tecnicofs-nosync tecnicofs-mutex tecnicofs-rwlock

.PHONY: all clean

all: $(TARGETS)


$(TARGETS):
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)


### no sync ###
lib/bst.o: lib/bst.c lib/bst.h
lib/hash.o: lib/hash.c lib/hash.h
fs.o: fs.c fs.h lib/bst.h lib/hash.h
sync.o: sync.c sync.h globals.h
main.o: main.c fs.h lib/bst.h globals.h lib/timer.h sync.h lib/hash.h
tecnicofs-nosync: lib/bst.o fs.o sync.o main.o lib/hash.o

### MUTEX ###
lib/bst-mutex.o: CFLAGS+=-DMUTEX
lib/bst-mutex.o: lib/bst.c lib/bst.h

fs-mutex.o: CFLAGS+=-DMUTEX
fs-mutex.o: fs.c fs.h lib/bst.h lib/hash.h

sync-mutex.o: CFLAGS+=-DMUTEX
sync-mutex.o: sync.c sync.h globals.h

main-mutex.o: CFLAGS+=-DMUTEX
main-mutex.o: main.c fs.h lib/bst.h globals.h lib/timer.h sync.h lib/hash.h
tecnicofs-mutex: lib/bst-mutex.o fs-mutex.o sync-mutex.o main-mutex.o lib/hash.o

### RWLOCK ###
lib/bst-rwlock.o: CFLAGS+=-DRWLOCK
lib/bst-rwlock.o: lib/bst.c lib/bst.h

fs-rwlock.o: CFLAGS+=-DRWLOCK
fs-rwlock.o: fs.c fs.h lib/bst.h lib/hash.h

sync-rwlock.o: CFLAGS+=-DRWLOCK
sync-rwlock.o: sync.c sync.h globals.h

main-rwlock.o: CFLAGS+=-DRWLOCK
main-rwlock.o: main.c fs.h lib/bst.h globals.h lib/timer.h sync.h lib/hash.h
tecnicofs-rwlock: lib/bst-rwlock.o fs-rwlock.o sync-rwlock.o main-rwlock.o lib/hash.o


%.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo Cleaning...
	rm -f $(OBJS) $(TARGETS)
	@rm -f *.out core inputs/*.out
	@rm -f -r output/*
	@clear

run: all
	@./tester.sh tecnicofs-nosync 0 100
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
	@#make run

irun: all
	./tecnicofs-mutex inputs/test03.txt myout.out 9 2