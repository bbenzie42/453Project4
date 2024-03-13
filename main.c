#include <stdio.h>
#include <stdlib.h>
int BLOCKSIZE = 512;
int NUM_TEST_DISKS = 2;
int TOTAL_DISKS = 0;

/* This functions opens a regular UNIX file and designates the first nBytes of it as space for the emulated disk. 
If nBytes is not exactly a multiple of BLOCKSIZE then the disk size will be the closest multiple
of BLOCKSIZE that is lower than nByte (but greater than 0) 
If nBytes > BLOCKSIZE and there is already a file by the given filename, that file’s content may be overwritten. 
If nBytes is 0, an existing disk is opened, and the content must not be overwritten in this function. 
There is no requirement to maintain integrity of any file content beyond nBytes. 
The return value is negative on failure or a disk number on success. */

int openDisk(char *filename, int nBytes) {
    if(nBytes == 0) { return TOTAL_DISKS; }
    if(nBytes < BLOCKSIZE) { return -1; }
    
    FILE *diskFile = fopen(filename, "wr");
    int blockOffset = nBytes % BLOCKSIZE;
    if(blockOffset != 0) {
        nBytes = nBytes/BLOCKSIZE; //this is incorrect arithmetic i think...
    }
    if(nBytes > BLOCKSIZE && diskFile != NULL) {
        return TOTAL_DISKS; //eventually replaced with disk ID in the superblock given by diskFile
    }
}

int closeDisk(int disk) {

}

int readBlock(int disk, int bNum, void* block) {

}

int writeBlock(int disk, int bNum, void* block) {

}

int main()
{
    int index = 0;
    char *diskName = "fillerdisk.dsk";
    char *buffer;
    for(index = 0; index < NUM_TEST_DISKS; index++) {
        buffer = malloc(BLOCKSIZE * sizeof(char));
    }
    printf("Hello world!\n");
    return 0;
}
