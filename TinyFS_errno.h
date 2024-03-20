#ifndef INC_453PROJECT4_TINYFS_ERRNO_H
#define INC_453PROJECT4_TINYFS_ERRNO_H

// Define error codes
#define E_SUCCESS 0
#define E_OPEN_DISK -1
#define E_READ_BLOCK -2
#define E_WRITE_BLOCK -3
#define E_MOUNT_FS -4
#define E_UNMOUNT_FS -5
#define E_NO_MOUNTED_DISK -6
#define E_WRONG_FS -7
#define E_CREATE_FILE -8
#define E_OPEN_FILE -9
#define E_CLOSE_FILE -10
#define E_WRITE_FILE -11
#define E_READ_FILE -12
#define E_REMOVE_FILE -13
#define E_FILE_NOT_FOUND -14
#define E_FILE_ALREADY_EXISTS -15
#define E_DISK_FULL -16
#define E_FILE_TOO_BIG -17
#define E_DELETE_FILE -18
#define E_SEEK_FILE -19
#define E_FREE_BLOCK -20

#endif //INC_453PROJECT4_TINYFS_ERRNO_H