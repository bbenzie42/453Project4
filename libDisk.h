//
// Created by pauls on 3/18/2024.
//



#ifndef INC_453PROJECT4_LIBDISK_H
#define INC_453PROJECT4_LIBDISK_H

#endif //INC_453PROJECT4_LIBDISK_H

int openDisk(char *filename, int nBytes);
int closeDisk(int disk); /* self explanatory */
int readBlock(int disk, int bNum, void *block);
int writeBlock(int disk, int bNum, void *block);
