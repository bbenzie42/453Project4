#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TinyFS_errno.h"
#include "libDisk.h"
#include "tinyFS.h"

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
   remove_blocks(&inode.file_extent);

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

   // Retrieve the inode number of the file from resource_table
   int inode_num = resource_table[FD].inode;
   inode_t inode;
   if (readBlock(mounted_disk, inode_num, (char*)&inode) != E_SUCCESS) {
      return E_READ_BLOCK; // Error reading block
   }

   // Traverse the entire file block list and free up each block
   int current_block = inode.file_extent;
   file_extent_t file_block;
   while(current_block != -1){
      if(readBlock(mounted_disk, current_block, (char*)&file_block) != E_SUCCESS){
         return E_READ_BLOCK; // Error reading block
      }
      freeBlock(current_block);
      current_block = file_block.next_block;
   }

   // Mark the inode block as free
   // Assume that you have a freeBlock function that marks a block as free
   freeBlock(inode_num);

   resource_table[FD].filename = NULL; // Mark the file as removed from the resource_table

   return E_SUCCESS; // File deleted successfully
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



int find_file(char* name) {
   // Traverse some data structure that stores file name to inode mappings
   // Return the inode if found, -1 if not found
   // Placeholder return
   return -1;
}int create_file(char* name) {
   // Allocate space for the new file
   // Create an entry in your file to inode mapping
   // Return the inode of the new file
   // Placeholder return
   return -1;
}int* allocate_blocks(int num_blocks) {
   // Implement block allocation logic here
   // Placeholder return
   return NULL;
}void remove_blocks(int* blocks_start) {
   // Implement block deallocation logic here
}void freeBlock(int block_number) {
   // Mark the block as free in your block allocation table
}

/* TinyFS demo file
 *  * Foaad Khosmood, Cal Poly / modified Winter 2014
 *   */


/* simple helper function to fill Buffer with as many inPhrase strings as possible before reaching size */
int
fillBufferWithPhrase (char *inPhrase, char *Buffer, int size)
{
   int index = 0, i;
   if (!inPhrase || !Buffer || size <= 0 || size < strlen (inPhrase))
      return -1;

   while (index < size)
   {
      for (i = 0; inPhrase[i] != '\0' && (i + index < size); i++)
         Buffer[i + index] = inPhrase[i];
      index += i;
   }
   Buffer[size - 1] = '\0';	/* explicit null termination */
   return 0;
}

/* This program will create 2 files (of sizes 200 and 1000) to be read from or stored in the TinyFS file system. */
/*
int
main ()
{
   char readBuffer;
   char *afileContent, *bfileContent;	*/
/* buffers to store file content *//*

   int afileSize = 200;		*/
/* sizes in bytes *//*

   int bfileSize = 1000;

   char phrase1[] = "hello world from (a) file ";
   char phrase2[] = "(b) file content ";

   fileDescriptor aFD, bFD;
   int i, returnValue;

*/
/* try to mount the disk *//*

   if (tfs_mount (DEFAULT_DISK_NAME) < 0)	*/
/* if mount fails *//*

   {
      tfs_mkfs (DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE);	*/
/* then make a new disk *//*

      if (tfs_mount (DEFAULT_DISK_NAME) < 0)	*/
/* if we still can't open it... *//*

      {
         perror ("failed to open disk");	*/
/* then just exit *//*

         return;
      }
   }


   afileContent = (char *) malloc (afileSize * sizeof (char));
   if (fillBufferWithPhrase (phrase1, afileContent, afileSize) < 0)
   {
      perror ("failed");
      return;
   }

   bfileContent = (char *) malloc (bfileSize * sizeof (char));
   if (fillBufferWithPhrase (phrase2, bfileContent, bfileSize) < 0)
   {
      perror ("failed");
      return;
   }

*/
/* print content of files for debugging *//*

   printf
      ("(a) File content: %s\n(b) File content: %s\nReady to store in TinyFS\n",
       afileContent, bfileContent);


*/
/* read or write files to TinyFS *//*



   aFD = tfs_openFile ("afile");

   if (aFD < 0)
   {
      perror ("tfs_openFile failed on afile");
   }

*/
/* now, was there already a file named "afile" that had some content? If we can read from it, yes!
 *  * If we can't read from it, it presumably means the file was empty.
 *   * If the size is 0 (all new files are sized 0) then any "readByte()" should fail, so
 *    * it's a new file and empty *//*

   if (tfs_readByte (aFD, &readBuffer) < 0)
   {
      */
/* if readByte() fails, there was no afile, so we write to it *//*

      if (tfs_writeFile (aFD, afileContent, afileSize) < 0)
      {
         perror ("tfs_writeFile failed");
      }
      else
         printf ("Successfully written to afile\n");


   }
   else
   {
      */
/* if yes, then just read and print the rest of afile that was already there *//*

      printf ("\n*** reading afile from TinyFS: \n%c", readBuffer);	*/
/* print the first byte already read *//*

      */
/* now print the rest of it, byte by byte *//*

      while (tfs_readByte (aFD, &readBuffer) >= 0)	*/
/* go until readByte fails *//*

         printf ("%c", readBuffer);

      */
/* close file *//*

      if (tfs_closeFile (aFD) < 0)
         perror ("tfs_closeFile failed");

      */
/* now try to delete the file. It should fail because aFD is no longer valid *//*

      if (tfs_deleteFile (aFD) < 0)
      {
         aFD = tfs_openFile ("afile");	*/
/* so we open it again *//*

         if (tfs_deleteFile (aFD) < 0)
            perror ("tfs_deleteFile failed");

      }
      else
         perror ("tfs_deleteFile should have failed");

   }

*/
/* now bfile tests *//*

   bFD = tfs_openFile ("bfile");

   if (bFD < 0)
   {
      perror ("tfs_openFile failed on bfile");
   }

   if (tfs_readByte (bFD, &readBuffer) < 0)
   {
      if (tfs_writeFile (bFD, bfileContent, bfileSize) < 0)
      {
         perror ("tfs_writeFile failed");
      }
      else
         printf ("Successfully written to bfile\n");
   }
   else
   {
      printf ("\n*** reading bfile from TinyFS: \n%c", readBuffer);
      while (tfs_readByte (bFD, &readBuffer) >= 0)
         printf ("%c", readBuffer);

      tfs_deleteFile (bFD);
   }

*/
/* Free both content buffers *//*

   free (bfileContent);
   free (afileContent);
   if (tfs_unmount () < 0)
      perror ("tfs_unmount failed");

   printf ("\nend of demo\n\n");
   return 0;
}*/
