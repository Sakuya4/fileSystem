#ifndef _INODE_H_
#define _INODE_H_

#include "types.h"
#include <time.h>

struct super_block;

typedef enum 
{
  FS_INODE_FILE = 1,
  FS_INODE_DIR  = 2,
} fs_inode_type_t;

struct inode 
{
  fs_ino_t      i_ino;
  fs_inode_type_t i_type;
  fs_mode_t     i_mode;
  fs_uid_t      i_uid;
  fs_gid_t      i_gid;
  fs_nlink_t    i_nlink;
  fs_off_t      i_size;

  struct timespec i_atime;
  struct timespec i_mtime;
  struct timespec i_ctime;
  
  struct super_block *i_sb;
};

#endif /* _INODE_H_ */
