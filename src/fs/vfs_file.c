/* standar library */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
/* standar library done */

/* user define library */
#include "vfs.h"
#include "vfs_internal.h"
#include "inode.h"
#include "dentry.h"
#include "path.h"
/* user define library done */

/* user define function */
int vfs_cat(const char *path);
int vfs_write_all(const char *path, const char *data);
int vfs_create_file(const char *path);
/* user define function done */

/* function */

int vfs_cat(const char *path)
{
  struct dentry *dent;
  struct inode  *inode;

  if (!path)
    return -1;

  dent = vfs_lookup(path);
  if (!dent || !dent->d_inode)
    return -1;

  inode = dent->d_inode;
  if (inode->i_type != FS_INODE_FILE)
    return -1;

  if (inode->i_data)
    printf("%s\n", inode->i_data);

  return 0;
}

int vfs_write_all(const char *path, const char *data)
{
  struct dentry *dent;
  struct inode  *inode;
  size_t len;
  char *buf;

  if (!path || !data)
    return -1;

  dent = vfs_lookup(path);
  if (!dent || !dent->d_inode)
    return -1;

  inode = dent->d_inode;

  if (inode->i_type != FS_INODE_FILE)
    return -1;

  len = strlen(data);
  buf = malloc(len + 1);
  if (!buf)
    return -1;

  memcpy(buf, data, len + 1);

  if (inode->i_data)
    free(inode->i_data);

  inode->i_data      = buf;
  inode->i_data_size = len;
  inode->i_data_cap  = len + 1;
  inode->i_size      = (fs_off_t)len;
  inode->i_mtime     = (uint64_t)time(NULL);

  return 0;
}

int vfs_create_file(const char *path)
{
  struct inode  *inode;
  struct dentry *parent;
  struct dentry *dentry;

  char buf[256];
  char *last_slash;
  char *name;

  if (!path || path[0] == '\0')
    return -1;

  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  trim(buf);
  remove_multiple_slashes(buf);
  rstrip_slash(buf);

  if (buf[0] == '\0')
    return -1;
  if (strcmp(buf, "/") == 0)
    return -1;

  last_slash = strrchr(buf, '/');

  if (last_slash == NULL)
  {
    parent = fs_get_cwd_dentry();
    name   = buf;
  }
  else
  {
    if (last_slash == buf)
    {
      parent = fs_get_super()->s_root;
      name   = last_slash + 1;
    }
    else
    {
      *last_slash = '\0';
      name        = last_slash + 1;

      parent = vfs_lookup(buf);
      if (!parent)
        return -1;
    }
  }

  if (name[0] == '\0')
    return -1;
  if (!parent || !parent->d_inode)
    return -1;
  if (parent->d_inode->i_type != FS_INODE_DIR)
    return -1;

  if (dentry_find_child(parent, name) != NULL)
    return -1;

  inode = calloc(1, sizeof(struct inode));
  if (!inode)
    return -1;

  inode->i_ino   = 0;
  inode->i_type  = FS_INODE_FILE;
  inode->i_mode  = FS_IFREG | 0644;
  inode->i_uid   = 0;
  inode->i_gid   = 0;
  inode->i_nlink = 1;
  inode->i_size  = 0;
  inode->i_mtime = (uint64_t)time(NULL);

  inode->i_data      = NULL;
  inode->i_data_size = 0;
  inode->i_data_cap  = 0;

  dentry = calloc(1, sizeof(struct dentry));
  if (!dentry)
  {
    free(inode);
    return -1;
  }

  dentry->d_name = fs_strdup(name);
  if (!dentry->d_name)
  {
    free(dentry);
    free(inode);
    return -1;
  }
  dentry->d_inode = inode;

  if (dentry_add_child(parent, dentry) != 0)
  {
    free(dentry->d_name);
    free(dentry);
    free(inode);
    return -1;
  }
  return 0;
}


/* function done */
