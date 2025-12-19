#ifndef _VFS_H_
#define _VFS_H_

#include "super.h"

int fs_init(void);
struct super_block *fs_get_super(void);
struct dentry *vfs_lookup(const char *path);

int vfs_mkdir(const char *path);
int vfs_ls(void);
int vfs_ls_path(const char *path);
int vfs_cd(const char *path);
int vfs_get_cwd(char *buf, size_t size);
int vfs_chmod(const char *path, int mode777);

int vfs_create_file(const char *path);  /*touch*/
int vfs_write_all(const char *path, const char *data);
int vfs_cat(const char *path);

int vfs_rm(const char *path);
int vfs_rmdir(const char *path);

int vfs_ls_long(void);
int vfs_ls_long_path(const char *path);

int vfs_vim(const char *path);

int vfs_import(const char *host_path, const char *vfs_path);
int vfs_export(const char *vfs_path, const char *host_path);

#endif /* _VFS_H_ */