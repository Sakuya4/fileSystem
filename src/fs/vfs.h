#ifndef _VFS_H_
#define _VFS_H_

#include "super.h"

int fs_init(void);

struct super_block *fs_get_super(void);

#endif /* _VFS_H_ */