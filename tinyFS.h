//
// Created by pauls on 3/18/2024.
//

#ifndef INC_453PROJECT4_TINYFS_H
#define INC_453PROJECT4_TINYFS_H

#define MAX_OPEN_FILES 128
#define DEFAULT_DISK_SIZE 10240
#define DEFAULT_DISK_NAME “tinyFSDisk”
typedef int fileDescriptor;

#define BLOCKSIZE 256
#define MAGIC_NUMBER 0x44

typedef struct superblock {
   unsigned char block_type;
   unsigned char magic_number;
   int root_inode;
   int free_block_list;
   char padding[BLOCKSIZE - 2 - sizeof(int)*2];
} superblock_t;

typedef struct inode {
   unsigned char block_type;
   unsigned char magic_number;
   char file_name[9]; // 8 characters + NULL terminator
   int file_size;
   int file_extent;
   char padding[BLOCKSIZE - 2 - 9 - sizeof(int)*2];
} inode_t;

typedef struct file_extent {
   unsigned char block_type;
   unsigned char magic_number;
   int next_block; // block# of next file extent or inode
   char data[BLOCKSIZE - sizeof(int) - 2]; // rest space for data
} file_extent_t;

typedef struct free_block {
   unsigned char block_type;
   unsigned char magic_number;
   int next_free_block; // block# of next free block
   char reserved[BLOCKSIZE - sizeof(int) - 2]; // rest space as reserved
} free_block_t;

#endif //INC_453PROJECT4_TINYFS_H
