#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TinyFS_errno.h"
#include "tinyFS.h"
#include "libDisk.h"

fileDescriptor mountedFile; //for currently mounted file
int mounted_disk = -1;


// Global variable for next file descriptor to allocate
fileDescriptor next_fd = 0;

typedef struct FileEntry {
   char *filename;
   int file_pointer;
   int inode;
} FileEntry;

FileEntry resource_table[MAX_OPEN_FILES];


/* Makes a blank TinyFS file system of size nBytes on the unix file
specified by ‘filename’. This function should use the emulated disk
library to open the specified unix file, and upon success, format the
file to be a mountable disk. This includes initializing all data to 0x00,
setting magic numbers, initializing and writing the superblock and
inodes, etc. Must return a specified success/error code. */

int tfs_mkfs(char *filename, int nBytes) {
   // Open the Unix file with our block device emulator
   int diskId = openDisk(filename, nBytes);
   if(diskId < 0) {
      return diskId; // Propagate the error
   }

   // Initialize and write the superblock
   struct superblock sb;
   sb.block_type = 1;
   sb.magic_number = 0x44;

   // Root inode and free block list initialized to -1 as they don't exist yet
   sb.root_inode = -1;
   sb.free_block_list = -1;

   // Write the superblock to the first block of the disk
   if(writeBlock(diskId, 0, &sb) != E_SUCCESS) {
      return E_WRITE_BLOCK; // Error writing block
   }

   // Success
   return E_SUCCESS;
}


/* tfs_mount(char *diskname) “mounts” a TinyFS file system located within
‘diskname’. tfs_unmount(void) “unmounts” the currently mounted file
system. As part of the mount operation, tfs_mount should verify the file
system is the correct type. In tinyFS, only one file system may be
mounted at a time. Use tfs_unmount to cleanly unmount the currently
mounted file system. Must return a specified success/error code. */
int tfs_mount(char *diskname) {
   // The disk drive must be already created, so 0 bytes for size
   int diskId = openDisk(diskname, 0);

   // Check for error opening the disk file
   if (diskId < 0) {
      return E_OPEN_DISK;
   }

   // Now read the first block and check if magic number is correct
   superblock_t sb;
   if (readBlock(diskId, 0, &sb) != E_SUCCESS) {
      return E_READ_BLOCK;
   }

   if (sb.magic_number != MAGIC_NUMBER) {
      return E_WRONG_FS; // Wrong filesystem type
   }

   // Everything seems OK, "mount" the disk
   mounted_disk = diskId;
   return E_SUCCESS;
}

int tfs_unmount(void) {
   // Check if there's a mounted disk
   if (mounted_disk < 0) {
      return E_NO_MOUNTED_DISK;
   }

   // Possible clean-up operations removed for brevity

   // "Unmount" the disk
   mounted_disk = -1;
   return E_SUCCESS;
}


/* Creates or Opens a file for reading and writing on the currently
mounted file system. Creates a dynamic resource table entry for the file,
and returns a file descriptor (integer) that can be used to reference
this entry while the filesystem is mounted. */

fileDescriptor tfs_openFile(char *name) {
   // Check if the file already exists
   int inode = find_file(name);
   if (inode < 0) {
      // File doesn't exist, create it
      inode = create_file(name);
      if (inode < 0) {
         return inode; // Propagate the error
      }
   }

   // Check the resource table for available entry
   if (next_fd >= MAX_OPEN_FILES) {
      return E_OPEN_FILE; // No available entry in resource table
   }

   // Add entry to resource table
   resource_table[next_fd].filename = name; // Assume the name is statically allocated
   resource_table[next_fd].inode = inode;

   // Return the file descriptor
   return next_fd++;
}

int tfs_closeFile(fileDescriptor FD) {
   // Check for valid file descriptor
   if (FD < 0 || FD >= next_fd) {
      return E_CLOSE_FILE; // Invalid file descriptor
   }

   // De-allocate system resources
   // (Depends on what resources are associated with open files in the filesystem)

   // Remove entry from resource table by simply marking it as available for reuse
   // (Assume closed file descriptors can be reused)
   if (FD == next_fd - 1) {
      next_fd--; // Reuse immediately next time
   } else {
      // A gap will be formed in the array, handle it according to your design choice (could use an explicit "is available" flag for each entry, or a linked list to track available entries, etc.)
   }

   return E_SUCCESS;
}


/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire
file’s content, to the file system. Previous content (if any) will be
completely lost. Sets the file pointer to 0 (the start of file) when
done. Returns success/error codes. */

int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
   // Check for a valid file descriptor
   if (FD < 0 || FD >= next_fd) {
      return E_WRITE_FILE; // Invalid file descriptor
   }

   // Get the inode of the file
   int inode_num = resource_table[FD].inode;
   inode_t inode;
   if (readBlock(mounted_disk, inode_num, &inode) != E_SUCCESS) {
      return E_READ_BLOCK; // Error reading block
   }

   // Calculate required number of blocks for the file content
   int num_blocks = (size + BLOCKSIZE - 1) / BLOCKSIZE;

   // Allocate new blocks for the file content
   int* new_blocks = allocate_blocks(num_blocks);
   if (new_blocks == NULL) {
      return E_DISK_FULL; // Cannot allocate enough blocks
   }

   // Remove the old blocks of the file
   remove_blocks(inode.file_extent);

   // Set the updated file size and first block of the file extent in inode
   inode.file_size = size;
   inode.file_extent = new_blocks[0];

   // Write the updated inode back to the disk
   if (writeBlock(mounted_disk, inode_num, &inode) != E_SUCCESS) {
      return E_WRITE_BLOCK; // Error writing block
   }

   // Write the file content to the allocated blocks
   for (int i = 0; i < num_blocks; i++) {
      file_extent_t extent;

      // If it's not the last block, link it to the next block
      if (i < num_blocks - 1) {
         extent.next_block = new_blocks[i + 1];
      } else {
         extent.next_block = -1; // This is the last block
      }

      // Copy the data to the block
      int bytes_to_copy = size > BLOCKSIZE ? BLOCKSIZE : size;
      memcpy(extent.data, buffer, bytes_to_copy);
      buffer += bytes_to_copy;
      size -= bytes_to_copy;

      // Write the block to the disk
      if (writeBlock(mounted_disk, new_blocks[i], &extent) != E_SUCCESS) {
         return E_WRITE_BLOCK; // Error writing block
      }
   }

   return E_SUCCESS;
}

/* deletes a file and marks its blocks as free on disk. */

int tfs_deleteFile(fileDescriptor FD) {
   // Check for a valid file descriptor
   if (FD < 0 || FD >= next_fd) {
      return E_DELETE_FILE; // Invalid file descriptor
   }

   // Get the inode of the file
   int inode_num = resource_table[FD].inode;
   inode_t inode;
   if (readBlock(mounted_disk, inode_num, &inode) != E_SUCCESS) {
      return E_READ_BLOCK; // Error reading block
   }

   // Remove the blocks of the file
   if (remove_blocks(inode.file_extent) != E_SUCCESS) {
      return E_DELETE_FILE; // Error removing blocks
   }

   // Mark the inode block as free
   if (mark_block_as_free(inode_num) != E_SUCCESS) {
      return E_DELETE_FILE; // Error marking block as free
   }

   // Remove the file descriptor from the resource table
   resource_table[FD].filename = NULL; // Mark as removed

   return E_SUCCESS; // Return with success
}

/* reads one byte from the file and copies it to buffer, using the
current file pointer location and incrementing it by one upon success.
If the file pointer is already past the end of the file then
tfs_readByte() should return an error and not increment the file pointer.
*/
int tfs_readByte(fileDescriptor FD, char *buffer) {
   // Check for a valid file descriptor
   if (FD < 0 || FD >= next_fd) {
      return E_READ_FILE; // Invalid file descriptor
   }

   // Get the inode of the file
   int inode_num = resource_table[FD].inode;
   inode_t inode;
   if (readBlock(mounted_disk, inode_num, &inode) != E_SUCCESS) {
      return E_READ_BLOCK; // Error reading block
   }

   // Check file pointer position
   if (resource_table[FD].file_pointer >= inode.file_size) {
      return E_READ_FILE; // Read position is past end of file
   }

   // Calculate which block to read
   int block_num = resource_table[FD].file_pointer / BLOCKSIZE;

   // Read this block
   char block[BLOCKSIZE];
   if (readBlock(mounted_disk, inode.file_extent + block_num, block) != E_SUCCESS) {
      return E_READ_BLOCK; // Error reading block
   }

   // Calculate relative position within the block
   int block_pos = resource_table[FD].file_pointer % BLOCKSIZE;

   // Read one byte
   *buffer = block[block_pos];

   // Increment file pointer
   resource_table[FD].file_pointer++;

   return E_SUCCESS; // Return with success
}

/* change the file pointer location to offset (absolute). Returns
success/error codes.*/
//this should just be a fseek call
int tfs_seek(fileDescriptor FD, int offset) {
   // Check for a valid file descriptor
   if(FD < 0 || FD >= next_fd) {
      return E_SEEK_FILE; // Invalid file descriptor
   }

   // Get the inode of the file
   int inode_num = resource_table[FD].inode;
   inode_t inode;
   if(readBlock(mounted_disk, inode_num, &inode) != E_SUCCESS) {
      return E_READ_BLOCK; // Error reading block
   }

   // Check if offset is within the bounds of the file
   if(offset < 0 || offset > inode.file_size) {
      return E_SEEK_FILE; // Offset is out of bounds
   }

   // Change the file pointer location to offset
   resource_table[FD].file_pointer = offset;

   return E_SUCCESS; // Return with success
}