//
// Created by pauls on 3/18/2024.
//



#ifndef INC_453PROJECT4_LIBDISK_H
#define INC_453PROJECT4_LIBDISK_H

#define NUM_TEST_DISKS 2
#define BLOCKSIZE 256
#define NUM_BLOCKS 50 /* total number of blocks on each disk */
#define NUM_TEST_BLOCKS 10
#define TEST_BLOCKS {25,39,8,9,15,21,25,33,35,42}

extern int TOTAL_DISKS;
extern FILE* disksFPs[NUM_TEST_DISKS];

int openDisk(char *filename, int nBytes);
int closeDisk(int disk); /* self explanatory */
int readBlock(int disk, int bNum, void *block);
int writeBlock(int disk, int bNum, void *block);
#endif //INC_453PROJECT4_LIBDISK_H


