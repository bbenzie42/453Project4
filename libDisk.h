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

#define TOTAL_DISKS 3
extern FILE* disksFPs[TOTAL_DISKS];
int openDisk(char *filename, int nBytes);
int closeDisk(int disk);
int readBlock(int disk, int bNum, void *block);
int writeBlock(int disk, int bNum, void *block);

int find_file(const char* name);
int create_file(const char* name);
int* allocate_blocks(int num_blocks);
void remove_blocks(int* blocks_start);
void freeBlock(int current_block);

#endif //INC_453PROJECT4_LIBDISK_H


