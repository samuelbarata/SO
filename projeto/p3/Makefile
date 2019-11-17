# Makefile, versao 2.1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

SOURCES = main.c fs.c sync.c
SOURCES+= lib/bst.c lib/hash.c lib/inodes.c
OBJS_NOSYNC = $(SOURCES:%.c=%.o)
OBJS_MUTEX  = $(SOURCES:%.c=%-mutex.o)
OBJS_RWLOCK = $(SOURCES:%.c=%-rwlock.o)
OBJS = $(OBJS_NOSYNC) $(OBJS_MUTEX) $(OBJS_RWLOCK)
CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../ -g
LDFLAGS=-lm -pthread
TARGETS = tecnicofs-nosync tecnicofs-mutex tecnicofs-rwlock

.PHONY: all clean

all: $(TARGETS)


$(TARGETS):
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)


### no sync ###
lib/bst.o: lib/bst.c lib/bst.h
lib/hash.o: lib/hash.c lib/hash.h
lib/inodes.o: lib/inodes.c lib/inodes.h
fs.o: fs.c fs.h lib/bst.h lib/hash.h
sync.o: sync.c sync.h globals.h
main.o: main.c fs.h lib/bst.h globals.h lib/timer.h sync.h lib/hash.h lib/inodes.h tecnicofs-api-constants.h tecnicofs-client-api.h
tecnicofs-nosync: lib/bst.o fs.o sync.o main.o lib/hash.o lib/inodes.o

### MUTEX ###
lib/bst-mutex.o: CFLAGS+=-DMUTEX
lib/bst-mutex.o: lib/bst.c lib/bst.h

fs-mutex.o: CFLAGS+=-DMUTEX
fs-mutex.o: fs.c fs.h lib/bst.h lib/hash.h

sync-mutex.o: CFLAGS+=-DMUTEX
sync-mutex.o: sync.c sync.h globals.h

main-mutex.o: CFLAGS+=-DMUTEX
main-mutex.o: main.c fs.h lib/bst.h globals.h lib/timer.h sync.h lib/hash.h lib/inodes.h tecnicofs-api-constants.h tecnicofs-client-api.h
tecnicofs-mutex: lib/bst-mutex.o fs-mutex.o sync-mutex.o main-mutex.o lib/hash.o lib/inodes.o

### RWLOCK ###
lib/bst-rwlock.o: CFLAGS+=-DRWLOCK
lib/bst-rwlock.o: lib/bst.c lib/bst.h

fs-rwlock.o: CFLAGS+=-DRWLOCK
fs-rwlock.o: fs.c fs.h lib/bst.h lib/hash.h

sync-rwlock.o: CFLAGS+=-DRWLOCK
sync-rwlock.o: sync.c sync.h globals.h

main-rwlock.o: CFLAGS+=-DRWLOCK
main-rwlock.o: main.c fs.h lib/bst.h globals.h lib/timer.h sync.h lib/hash.h lib/inodes.h tecnicofs-api-constants.h tecnicofs-client-api.h
tecnicofs-rwlock: lib/bst-rwlock.o fs-rwlock.o sync-rwlock.o main-rwlock.o lib/hash.o lib/inodes.o

%.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo Cleaning...
	rm -f $(OBJS) $(TARGETS)

cleaner: 
	@echo Cleaning...
	rm -f $(OBJS) $(TARGETS)
	@rm -f *.out core inputs/*.out .gdb_history *.o
	@rm -f -r output/
	@clear

run: all
	./runTests.sh inputs outputs 20 7

generator: SHELL:=/bin/bash
generator: tester.py check.py all
	@rm -rf myinp 2> /dev/null
	@mkdir myinp
	@echo Running test...
	@python3 tester.py 20
	@./tecnicofs-rwlock myinp/test6.txt myinp/test6.out 25 40 > myinp/test6.stdout
	@grep "^Tecnico" myinp/test6.stdout
	@python3 check.py myinp/temp
	@#rm -rf myinp
