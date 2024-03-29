#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TinyFS_errno.h"

#define NUM_TEST_DISKS 2
#define BLOCKSIZE 256
#define NUM_BLOCKS 50 /* total number of blocks on each disk */
#define NUM_TEST_BLOCKS 10
#define TEST_BLOCKS {25,39,8,9,15,21,25,33,35,42}

int TOTAL_DISKS = 0;
FILE* disksFPs[NUM_TEST_DISKS] = {0};

/* This functions opens a regular UNIX file and designates the first nBytes of it as space for the emulated disk. 
If nBytes is not exactly a multiple of BLOCKSIZE then the disk size will be the closest multiple
of BLOCKSIZE that is lower than nByte (but greater than 0) 
If nBytes > BLOCKSIZE and there is already a file by the given filename, that file’s content may be overwritten. 
If nBytes is 0, an existing disk is opened, and the content must not be overwritten in this function. 
There is no requirement to maintain integrity of any file content beyond nBytes. 
The return value is negative on failure or a disk number on success. */

int openDisk(char *filename, int nBytes) {
   FILE *diskFile = NULL;
   if(nBytes == 0) {
      diskFile = fopen(filename, "rb+");
      if(diskFile == NULL) return E_OPEN_DISK;
   }
   else {
      if(nBytes < BLOCKSIZE) { return E_READ_BLOCK; }
      diskFile = fopen(filename, "wb+");
      if(diskFile == NULL) return E_OPEN_DISK;

      int blockOffset = nBytes % BLOCKSIZE;
      if(blockOffset != 0) {
         nBytes -= blockOffset;
      }

      // Initalize the blocks with '0' value
      char* buffer = (char*)calloc(nBytes, 1);
      fwrite(buffer, 1, nBytes, diskFile);
      free(buffer);
   }

   // Store file pointer and increase disk count
   disksFPs[TOTAL_DISKS] = diskFile;
   return TOTAL_DISKS++;
}
int closeDisk(int disk) {
   // Check if the disk number is valid
   if(disk < 0 || disk >= TOTAL_DISKS) {
      return -1;
   }

   // Check if a disk file is open for the given disk number
   FILE *diskFile = disksFPs[disk];
   if(diskFile == NULL) {
      return -1;
   }

   // Close the disk file
   fclose(diskFile);

   // Remove the disk file pointer from the array
   disksFPs[disk] = NULL;

   return 0; // Return success
}

int readBlock(int disk, int bNum, void* block) {
   // Check if the disk number is valid and disk is open
   if(disk < 0 || disk >= TOTAL_DISKS || disksFPs[disk] == NULL) {
      return E_OPEN_DISK; // Disk not available
   }

   FILE* readDisk = disksFPs[disk];
   if (fseek(readDisk, bNum*BLOCKSIZE, SEEK_SET) != 0) {
      return E_READ_BLOCK; // Seek error
   }
   int checkSize = fread(block, sizeof(char), BLOCKSIZE, readDisk);
   if(checkSize < BLOCKSIZE) {
      return E_READ_BLOCK; // Read error
   }
   return E_SUCCESS; // Success
}

int writeBlock(int disk, int bNum, void* block) {
   // Check if the disk number is valid and disk is open
   if(disk < 0 || disk >= TOTAL_DISKS || disksFPs[disk] == NULL) {
      return E_OPEN_DISK; // Disk not available
   }

   FILE* writeDisk = disksFPs[disk];
   if (fseek(writeDisk, bNum*BLOCKSIZE, SEEK_SET) != 0) {
      return E_WRITE_BLOCK; // Seek error
   }
   size_t writeSize = fwrite(block, sizeof(char), BLOCKSIZE, writeDisk);
   if(writeSize < BLOCKSIZE) {
      return E_WRITE_BLOCK; // Write error
   }
   return E_SUCCESS; // Success
}


int main()
{
    int index, index2, index3 = 0;
    int disks[NUM_TEST_DISKS]; /* holds disk numbers being tested here */
    int retValue = 0;
    int testBlocks[NUM_TEST_BLOCKS] = TEST_BLOCKS;

    char diskName[] = "diskX.dsk"; /* Unix file name for the disks */
    char *buffer;
    for(index = 0; index < NUM_TEST_DISKS; index++) {
        buffer = malloc(BLOCKSIZE * sizeof(char));
        diskName[4] = '0'+index;
        disks[index] = openDisk(diskName, 0);
        if (disks[index] < 0) {
            printf("] Open failed with (%i). Disk probably does not exist.\n",disks[index]);
	        disks[index] = openDisk(diskName, BLOCKSIZE * NUM_BLOCKS); /* create the disk */
	        if (disks[index] < 0) {
                printf("] openDisk() failed to create a disk. This should never happen. Exiting. \n");
		        exit(0);
	        }

            memset(buffer,'$',BLOCKSIZE);
            for (index2 = 0; index2 < NUM_TEST_BLOCKS; index2++)
            {
                retValue = writeBlock(disks[index], testBlocks[index2], buffer);
                if (retValue < 0)
		        {
		            printf("] Failed to write to block %i of disk %s. Exiting (%i).\n",testBlocks[index2],diskName,retValue);
		            exit(0);
		        }
                printf("] Successfully wrote to block %i of disk %s.\n",testBlocks[index2],diskName);
            }
        }
        else {
	        printf("] Existing disk %s opened.\n",diskName);
	        /* determine if the testBlocks contain the dollar Signal.
            * Check every single byte */
            for (index2 = 0; index2 < NUM_TEST_BLOCKS; index2++) {
		        if (readBlock(disks[index],testBlocks[index2],buffer) < 0) {
                    printf("] Failed to read block %i of disk %s. Exiting.\n",testBlocks[index2],diskName);
                    exit(0);
                }
		        for(index3 =0; index3 < BLOCKSIZE; index3++) {
                    if (buffer[index3] != '$') {
                        printf("] Failed. Byte #%i of block %i of disk %s was supposed to be a \"$\". Exiting\n.",
                               index3,testBlocks[index2],diskName);
                        exit(0);
                    }
                }
            }
            printf("] Previous writes were verified. Now, delete the .dsk files if you want to run this test again.\n");
       }
    }
    return 0;
}

