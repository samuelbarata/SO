#include "../tecnicofs-api-constants.h"
#include "../client/tecnicofs-client-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s sock_path\n", argv[0]);
        exit(0);
    }
    
    char readBuffer[4] = {0};

    assert(tfsMount(argv[1]) == 0);

    assert(tfsCreate("bcd", RW, READ) == 0 );

    assert(tfsRename("bcd", "cde") == 0);

    int fd = -1;
    assert((fd = tfsOpen("cde", RW)) == 0);

    assert(tfsWrite(fd, "hmm", 3) == 0);

    assert(tfsRead(fd, readBuffer, 4) == 3);

    puts(readBuffer);

    assert(tfsClose(fd) == 0);

    assert(tfsDelete("cde") == 0);

    assert(tfsUnmount() == 0);

    printf("\nSUCCESS\n\n");

    return 0;
}