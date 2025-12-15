#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "vfs.h"
#include "vfs_internal.h"
#include "path.h"
#include "inode.h"
#include "dentry.h"
#include "block.h"

/* user define function*/
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
/* user define function done*/

/* =========================
 *  lookup
 * ========================= */
struct dentry *vfs_lookup(const char *path)
{
  if (path == NULL || *path == '\0')
  {
    return NULL;
  }

  char buf[256];
  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  trim(buf);
  remove_multiple_slashes(buf);
  rstrip_slash(buf);

  if (buf[0] == '\0')
  {
    return NULL;
  }

  struct dentry *cur;
  struct super_block *sb = fs_get_super();

  if (buf[0] == '/')
  {
    /* 絕對路徑 */
    cur = sb->s_root;
    if (buf[1] == '\0')
    {
      return cur;
    }

    char *p = buf + 1;
    char *tok;

    while ((tok = next_token(&p)) != NULL)
    {
      if (strcmp(tok, ".") == 0)
      {
        continue;
      }
      else if (strcmp(tok, "..") == 0)
      {
        if (cur->d_parent)
          cur = cur->d_parent;
        continue;
      }

      cur = dentry_find_child(cur, tok);
      if (!cur)
      {
        return NULL;
      }
    }
    return cur;
  }
  else
  {
    /* 相對路徑 */
    cur = fs_get_cwd_dentry();
    if (!cur)
    {
      return NULL;
    }

    char *p = buf;
    char *tok;

    while ((tok = next_token(&p)) != NULL)
    {
      if (strcmp(tok, ".") == 0)
      {
        continue;
      }
      else if (strcmp(tok, "..") == 0)
      {
        if (cur->d_parent)
          cur = cur->d_parent;
        continue;
      }

      cur = dentry_find_child(cur, tok);
      if (!cur)
      {
        return NULL;
      }
    }
    return cur;
  }
}

/* =========================
 *  mkdir / rmdir / rm
 * ========================= */

static int vfs_mkdir_path_internal(const char *path)
{
  struct inode  *inode;
  struct dentry *parent;
  struct dentry *dentry;

  char buf[256];
  char *last_slash;
  char *name;

  if (!path || path[0] == '\0')
  {
    return -1;
  }

  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  trim(buf);
  remove_multiple_slashes(buf);
  rstrip_slash(buf);

  if (buf[0] == '\0')
  {
    return -1;
  }

  if (strcmp(buf, "/") == 0)
  {
    return -1;
  }

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
      /* "/name" */
      parent = fs_get_super()->s_root;
      name   = last_slash + 1;
    }
    else
    {
      *last_slash = '\0';
      name        = last_slash + 1;

      parent = vfs_lookup(buf);
      if (!parent)
      {
        return -1;
      }
    }
  }

  if (name[0] == '\0')
  {
    return -1;
  }

  if (!parent || !parent->d_inode)
  {
    return -1;
  }
  if (parent->d_inode->i_type != FS_INODE_DIR)
  {
    return -1;
  }

  if (dentry_find_child(parent, name) != NULL)
  {
    return -1;
  }

  inode = calloc(1, sizeof(struct inode));
  if (!inode)
  {
    return -1;
  }

  inode->i_ino  = 0;
  inode->i_type = FS_INODE_DIR;
  inode->i_mode = FS_IFDIR | 0755;

  inode->i_uid   = 0;
  inode->i_gid   = 0;
  inode->i_nlink = 1;
  inode->i_size  = 0;
  inode->i_mtime = (uint64_t)time(NULL);

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

int vfs_mkdir(const char *path)
{
  return vfs_mkdir_path_internal(path);
}

int vfs_rm(const char *path)
{
  char buf[256];
  struct dentry *dent;
  struct dentry *parent;
  struct inode  *inode;

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

  dent = vfs_lookup(buf);
  if (!dent || !dent->d_inode)
    return -1;

  inode  = dent->d_inode;
  parent = dent->d_parent;

  struct super_block *sb = fs_get_super();
  if (dent == sb->s_root || !parent || parent == dent)
    return -1;

  if (inode->i_type != FS_INODE_FILE)
    return -1;

  if (dentry_remove_child(parent, dent) != 0)
    return -1;

  for (int i = 0; i < DIRECT_BLOCKS; i++) {
    if (inode->i_block[i] >= 0) {
      block_free(inode->i_block[i]);
      inode->i_block[i] = -1;
    }
  }

  free(inode);

  if (dent->d_name)
    free(dent->d_name);
  free(dent);

  return 0;
}

int vfs_rmdir(const char *path)
{
  char buf[256];
  struct dentry *dent;
  struct dentry *parent;
  struct inode  *inode;

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

  dent = vfs_lookup(buf);
  if (!dent || !dent->d_inode)
    return -1;

  inode  = dent->d_inode;
  parent = dent->d_parent;

  struct super_block *sb = fs_get_super();
  if (dent == sb->s_root || !parent || parent == dent)
    return -1;

  if (inode->i_type != FS_INODE_DIR)
    return -1;

  if (dent->d_child != NULL)
    return -1;  /* not empty */

  if (dentry_remove_child(parent, dent) != 0)
    return -1;

  free(inode);

  if (dent->d_name)
    free(dent->d_name);
  free(dent);

  return 0;
}

/* =========================
 *  cd
 * ========================= */

int vfs_cd(const char *path)
{
  char buf[256];
  struct dentry *target;

  if (!path || path[0] == '\0')
    return -1;

  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  trim(buf);
  if (buf[0] == '\0')
    return -1;

  target = vfs_lookup(buf);
  if (!target || !target->d_inode)
    return -1;

  if (target->d_inode->i_type != FS_INODE_DIR)
    return -1;

  fs_set_cwd_dentry(target);
  return 0;
}

int vfs_chmod(const char *path, int mode777)
{
  struct dentry *dent;
  struct inode  *ino;

  if (!path) return -1;

  dent = vfs_lookup(path);
  if (!dent || !dent->d_inode) return -1;

  ino = dent->d_inode;
  
  ino->i_mode = (ino->i_mode & FS_IFMT) | (mode777 & 0777);
  ino->i_mtime = (uint64_t)time(NULL);

  return 0;
}
