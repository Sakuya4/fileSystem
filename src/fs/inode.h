#ifndef _INODE_H_
#define _INODE_H_

#include "types.h"
#include <time.h>

/* define mode bit */
#define FS_IFMT   0170000
#define FS_IFREG  0100000
#define FS_IFDIR  0040000

#define FS_IRUSR  0400
#define FS_IWUSR  0200
#define FS_IXUSR  0100
#define FS_IRGRP  0040
#define FS_IWGRP  0020
#define FS_IXGRP  0010
#define FS_IROTH  0004
#define FS_IWOTH  0002
#define FS_IXOTH  0001
/* define mode bit done */

struct super_block;

typedef enum 
{
  FS_INODE_FILE = 1,
  FS_INODE_DIR  = 2,
} fs_inode_type_t;

struct inode 
{
  fs_ino_t      i_ino;    // inode number
  fs_inode_type_t i_type; // file or directory
  fs_mode_t     i_mode;   // 0755 etc. type bit   
  fs_uid_t      i_uid;
  fs_gid_t      i_gid;
  fs_nlink_t    i_nlink; // link count
  fs_off_t      i_size;  // file size in bytes
  uint64_t      i_mtime; // epoch time of last modification

  char *i_data;
  size_t i_data_size;
  size_t i_data_cap;

  // struct timespec i_atime;
  // struct timespec i_mtime;
  // struct timespec i_ctime;
  
  struct super_block *i_sb;
};

#endif /* _INODE_H_ */
