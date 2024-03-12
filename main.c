#include <stdio.h>
#include <stdlib.h>
int BLOCKSIZE = 512;
int NUM_TEST_DISKS = 2;

int openDisk(char *filename, int nBytes) {

}

int closeDisk(int disk) {

}

int main()
{
    int index = 0;
    char *diskName = "fillerdisk.dsk";
    char *buffer;
    for(index = 0; index < NUM_TEST_DISKS; i++) {
        buffer = malloc(BLOCKSIZE * sizeof(char));
    }
    printf("Hello world!\n");
    return 0;
}
