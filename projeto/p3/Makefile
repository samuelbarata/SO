# Makefile, versao 2.1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

SOURCES = server/main.c server/fs.c client/tecnicofs-client-api.c
SOURCES+= lib/bst.c lib/hash.c lib/inodes.c lib/safe.c lib/sync.c
OBJS_NOSYNC = $(SOURCES:%.c=%.o)
OBJS_MUTEX  = $(SOURCES:%.c=%-mutex.o)
OBJS_RWLOCK = $(SOURCES:%.c=%-rwlock.o)
OBJS = $(OBJS_NOSYNC) $(OBJS_MUTEX) $(OBJS_RWLOCK)
CC   = gcc
LD   = gcc
CFLAGS	=-Wall -std=gnu99 -I../ -g
LDFLAGS	=-lm -pthread
TARGETS	= tecnicofs-rwlock
API 	= client/tecnicofs-client-api.o
CLIENTS	= client-api-test-create client-api-test-delete client-api-test-read client-api-test-success client-killer

.PHONY: all clean debug runs runc cleaner clients zip

all: $(TARGETS) $(API)
clients: $(CLIENTS) $(API)

$(TARGETS):
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(CLIENTS): lib/safe.o $(API)
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(API):
	$(CC) $(CFLAGS) -c -o $@ $<


lib/safe.o: lib/safe.c lib/safe.h
lib/hash.o: lib/hash.c lib/hash.h
lib/inodes.o: lib/inodes.c lib/inodes.h

lib/bst-rwlock.o: CFLAGS+=-DRWLOCK
lib/bst-rwlock.o: lib/bst.c lib/bst.h

server/fs-rwlock.o: CFLAGS+=-DRWLOCK
server/fs-rwlock.o: server/fs.c server/fs.h lib/bst.h lib/hash.h

lib/sync-rwlock.o: CFLAGS+=-DRWLOCK
lib/sync-rwlock.o: lib/sync.c lib/sync.h globals.h

server/main-rwlock.o: CFLAGS+=-DRWLOCK
server/main-rwlock.o: server/main.c server/fs.h lib/safe.h lib/bst.h globals.h lib/timer.h lib/sync.h lib/hash.h lib/inodes.h tecnicofs-api-constants.h

tecnicofs-rwlock: lib/safe.o lib/bst-rwlock.o server/fs-rwlock.o lib/sync-rwlock.o server/main-rwlock.o lib/hash.o lib/inodes.o

client/tecnicofs-client-api.o: client/tecnicofs-client-api.c globals.h lib/safe.h client/tecnicofs-client-api.h tecnicofs-api-constants.h

client-api-test-create: api-tests/client-api-test-create.c
client-api-test-delete: api-tests/client-api-test-delete.c
client-api-test-read: api-tests/client-api-test-read.c
client-api-test-success: api-tests/client-api-test-success.c
client-killer: api-tests/client-killer.c

%.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo Cleaning...
	rm -f $(OBJS) $(TARGETS) $(CLIENTS) $(API)

cleaner: clean
	@rm -f *.out core inputs/*.out .gdb_history *.o proj.zip
	@rm -f -r output/
	@clear

runs: $(TARGETS)
	./tecnicofs-rwlock /tmp/mySocket myout.out 2

runc: $(API) $(CLIENTS)
	#./client-api-test-create /tmp/mySocket
	#./client-api-test-delete /tmp/mySocket
	#./client-api-test-read /tmp/mySocket
	#./client-api-test-success /tmp/mySocket
	./client-killer /tmp/mySocket

debug: CFLAGS+=-DDEBUG
debug: clean all $(CLIENTS)

zip: clean
	zip proj.zip *.c *.h lib/*.c lib/*.h client/*.c client/*.h server/*.c server/*.h Makefile
