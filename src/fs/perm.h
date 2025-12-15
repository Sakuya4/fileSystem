#ifndef _PERM_H_
#define _PERM_H_

#include "inode.h"

#define FS_R_OK 4
#define FS_W_OK 2
#define FS_X_OK 1

int fs_perm_check(const struct inode *ino, int need);

#endif
