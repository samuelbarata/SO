# Makefile, versao 2.1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

SOURCES = main.c fs.c sync.c tecnicofs-client-api.c
SOURCES+= lib/bst.c lib/hash.c lib/inodes.c lib/safe.c
OBJS_NOSYNC = $(SOURCES:%.c=%.o)
OBJS_MUTEX  = $(SOURCES:%.c=%-mutex.o)
OBJS_RWLOCK = $(SOURCES:%.c=%-rwlock.o)
OBJS = $(OBJS_NOSYNC) $(OBJS_MUTEX) $(OBJS_RWLOCK)
CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../ -g
LDFLAGS=-lm -pthread
TARGETS = tecnicofs-rwlock client-api-test-create.sh client-api-test-delete.sh client-api-test-read.sh client-api-test-success.sh

.PHONY: all clean

all: $(TARGETS)


$(TARGETS):
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)


lib/safe.o: lib/safe.c lib/safe.h

lib/hash.o: lib/hash.c lib/hash.h

lib/inodes.o: lib/inodes.c lib/inodes.h

lib/bst-rwlock.o: CFLAGS+=-DRWLOCK
lib/bst-rwlock.o: lib/bst.c lib/bst.h

fs-rwlock.o: CFLAGS+=-DRWLOCK
fs-rwlock.o: fs.c fs.h lib/bst.h lib/hash.h

sync-rwlock.o: CFLAGS+=-DRWLOCK
sync-rwlock.o: sync.c sync.h globals.h

tecnicofs-client-api.o: tecnicofs-client-api.c tecnicofs-client-api.h tecnicofs-api-constants.h 

client-api-test-create.sh: tecnicofs-client-api.o api-tests/client-api-test-create.c lib/safe.o
client-api-test-delete.sh: tecnicofs-client-api.o api-tests/client-api-test-delete.c lib/safe.o
client-api-test-read.sh: tecnicofs-client-api.o api-tests/client-api-test-read.c lib/safe.o
client-api-test-success.sh: tecnicofs-client-api.o api-tests/client-api-test-success.c lib/safe.o

main-rwlock.o: CFLAGS+=-DRWLOCK
main-rwlock.o: main.c fs.h lib/safe.h lib/bst.h globals.h lib/timer.h sync.h lib/hash.h lib/inodes.h tecnicofs-api-constants.h tecnicofs-client-api.h
tecnicofs-rwlock: lib/safe.o lib/bst-rwlock.o fs-rwlock.o sync-rwlock.o main-rwlock.o lib/hash.o lib/inodes.o

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

runs: all
	./tecnicofs-rwlock /tmp/mySocket myout.out 2

runc: client-api-test-create.sh client-api-test-delete.sh client-api-test-read.sh client-api-test-success.sh
	./client-api-test-create.sh /tmp/mySocket

debug: CFLAGS+=-DDEBUG
debug: clean all
	make runs
