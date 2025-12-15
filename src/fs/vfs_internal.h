#ifndef _VFS_INTERNAL_H_
#define _VFS_INTERNAL_H_

#include "super.h"
#include "inode.h"
#include "dentry.h"
#include "types.h"

struct super_block *fs_get_super(void);
struct dentry *fs_get_cwd_dentry(void);
void fs_set_cwd_dentry(struct dentry *d);

char *fs_strdup(const char *s);

int  dentry_add_child(struct dentry *parent, struct dentry *child);
int  dentry_remove_child(struct dentry *parent, struct dentry *child);
fs_uid_t fs_get_uid(void);     // get current user id
void fs_set_uid(fs_uid_t uid);
struct dentry *dentry_find_child(struct dentry *parent, const char *name);

#endif /* _VFS_INTERNAL_H_ */



