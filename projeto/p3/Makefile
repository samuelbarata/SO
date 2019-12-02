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

.PHONY: all clean debug runs runc cleaner clients zip valgrind

#compila server + API
all: $(TARGETS) $(API)

#compila clientes teste + API
clients: $(CLIENTS) $(API)

$(TARGETS):
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(CLIENTS): lib/safe.o $(API)
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(API):
	$(CC) $(CFLAGS) -c -o $@ $<


lib/safe.o: lib/safe.c lib/safe.h
lib/hash.o: lib/hash.c lib/hash.h
lib/inodes.o: lib/inodes.c lib/inodes.h lib/globals.h

lib/bst-rwlock.o: CFLAGS+=-DRWLOCK
lib/bst-rwlock.o: lib/bst.c lib/bst.h

server/fs-rwlock.o: CFLAGS+=-DRWLOCK
server/fs-rwlock.o: server/fs.c server/fs.h lib/bst.h lib/hash.h lib/globals.h lib/tecnicofs-api-constants.h

lib/sync-rwlock.o: CFLAGS+=-DRWLOCK
lib/sync-rwlock.o: lib/sync.c lib/sync.h lib/globals.h

server/main-rwlock.o: CFLAGS+=-DRWLOCK
server/main-rwlock.o: server/main.c server/fs.h lib/safe.h lib/bst.h lib/globals.h lib/timer.h lib/sync.h lib/hash.h lib/inodes.h lib/tecnicofs-api-constants.h

tecnicofs-rwlock: lib/safe.o lib/bst-rwlock.o server/fs-rwlock.o lib/sync-rwlock.o server/main-rwlock.o lib/hash.o lib/inodes.o

client/tecnicofs-client-api.o: client/tecnicofs-client-api.c lib/globals.h lib/safe.h client/tecnicofs-client-api.h lib/tecnicofs-api-constants.h

client-api-test-create: api-tests/client-api-test-create.c lib/tecnicofs-api-constants.h
client-api-test-delete: api-tests/client-api-test-delete.c lib/tecnicofs-api-constants.h
client-api-test-read: api-tests/client-api-test-read.c lib/tecnicofs-api-constants.h
client-api-test-success: api-tests/client-api-test-success.c lib/tecnicofs-api-constants.h
client-killer: api-tests/client-killer.c lib/tecnicofs-api-constants.h

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
	./client-killer /tmp/mySocket

debug: CFLAGS+=-DDEBUG
debug: clean all $(CLIENTS)

zip: cleaner
	zip proj.zip lib/*.c lib/*.h client/*.c client/*.h server/*.c server/*.h server/Makefile
valgrind: all
	valgrind --leak-check=full  ./tecnicofs-rwlock /tmp/mySocket myout.out 3
