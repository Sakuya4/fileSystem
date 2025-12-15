#ifndef _PERM_H_
#define _PERM_H_

#include "inode.h"
#include "types.h"

#define FS_R_OK  0x4
#define FS_W_OK  0x2
#define FS_X_OK  0x1

int fs_perm_check(const struct inode *inode, int mask);

fs_uid_t fs_get_uid(void);
void fs_set_uid(fs_uid_t uid);

#endif /* _PERM_H_ */
