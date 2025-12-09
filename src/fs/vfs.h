#ifndef _VFS_H_
#define _VFS_H_

#include "super.h"

int fs_init(void);
struct super_block *fs_get_super(void);

int vfs_mkdir(const char *name);
int vfs_ls(void);
int vfs_cd(const char *name);

#endif /* _VFS_H_ */