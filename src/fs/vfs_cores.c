#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "vfs.h"
#include "vfs_internal.h"
#include "inode.h"
#include "dentry.h"
#include "path.h"
#include "block.h"

/* global super block and cwd */
static struct super_block g_sb;
static struct dentry      *g_cwd;

/* --- getters / setters --- */

struct super_block *fs_get_super(void)
{
  return &g_sb;
}

struct dentry *fs_get_cwd_dentry(void)
{
  return g_cwd;
}

void fs_set_cwd_dentry(struct dentry *d)
{
  if (d)
  {
    g_cwd = d;
  }
}

/* --- small helpers shared by multiple files --- */

char *fs_strdup(const char *s)
{
  size_t len = strlen(s) + 1;
  char *p = malloc(len);
  if (p)
  {
    memcpy(p, s, len);
  }
  return p;
}

int dentry_add_child(struct dentry *parent, struct dentry *child)
{
  if (!parent || !child)
    return -1;

  child->d_parent  = parent;
  child->d_sibling = parent->d_child;
  parent->d_child  = child;
  return 0;
}

int dentry_remove_child(struct dentry *parent, struct dentry *child)
{
  struct dentry *prev = NULL;
  struct dentry *cur;

  if (!parent || !child)
    return -1;

  cur = parent->d_child;
  while (cur && cur != child)
  {
    prev = cur;
    cur  = cur->d_sibling;
  }

  if (!cur)
    return -1;

  if (!prev)
    parent->d_child = cur->d_sibling;
  else
    prev->d_sibling = cur->d_sibling;

  child->d_parent  = NULL;
  child->d_sibling = NULL;
  return 0;
}

struct dentry *dentry_find_child(struct dentry *parent, const char *name)
{
  struct dentry *cur;

  if (!parent || !name)
    return NULL;

  for (cur = parent->d_child; cur != NULL; cur = cur->d_sibling)
  {
    if (cur->d_name && strcmp(cur->d_name, name) == 0)
      return cur;
  }
  return NULL;
}

/* --- fs_init: 建 root inode + root dentry --- */

int fs_init(void)
{
  memset(&g_sb, 0, sizeof(g_sb));
  g_sb.s_magic = 0x12345678;

  struct inode *root_inode = malloc(sizeof(struct inode));
  if (!root_inode)
  {
    return -1;
  }

  memset(root_inode, 0, sizeof(*root_inode));
  root_inode->i_ino  = 1;
  root_inode->i_type = FS_INODE_DIR;

  root_inode->i_mode  = FS_IFDIR | FS_IRUSR | FS_IWUSR | FS_IXUSR |
                        FS_IRGRP | FS_IXGRP |
                        FS_IROTH | FS_IXOTH;  /* 0755 */
  root_inode->i_uid   = 0;
  root_inode->i_gid   = 0;
  root_inode->i_nlink = 1;
  root_inode->i_size  = 0;
  root_inode->i_mtime = (uint64_t)time(NULL);

  for (int i = 0; i < DIRECT_BLOCKS; i++)
  {
    root_inode->i_block[i] = -1; 
  }

  struct dentry *root_dentry = malloc(sizeof(struct dentry));
  if (!root_dentry)
  {
    free(root_inode);
    return -1;
  }

  memset(root_dentry, 0, sizeof(*root_dentry));
  root_dentry->d_name   = fs_strdup("/");
  root_dentry->d_parent = root_dentry; /* root 的 parent 指自己 */
  root_dentry->d_inode  = root_inode;

  g_sb.s_root = root_dentry;
  g_cwd       = root_dentry;
  block_init();
  return 0;
}

/* --- vfs_get_cwd: 提供給 shell 顯示 prompt --- */

int vfs_get_cwd(char *buf, size_t size)
{
  const char *names[64];
  int depth = 0;
  struct dentry *d;
  size_t pos = 0;
  int i;

  if (!buf || size == 0)
  {
    return -1;
  }

  buf[0] = '\0';

  if (!g_cwd)
  {
    snprintf(buf, size, "?");
    return -1;
  }

  /* root 特例 */
  if (g_cwd == g_sb.s_root)
  {
    snprintf(buf, size, "/");
    return 0;
  }

  /* 往上爬，把每層名稱存到 names[]（反向） */
  d = g_cwd;
  while (d && d != g_sb.s_root && depth < 64)
  {
    if (d->d_name)
      names[depth++] = d->d_name;
    else
      names[depth++] = "?";
    d = d->d_parent;
  }

  /* 前面先輸出一個 '/' */
  pos += snprintf(buf + pos, size - pos, "/");

  /* 再從最上層一路往下拼 "/home/user" */
  for (i = depth - 1; i >= 0; i--)
  {
    if (pos >= size)
    {
      buf[size - 1] = '\0';
      return -1;
    }

    if (i > 0)
      pos += snprintf(buf + pos, size - pos, "%s/", names[i]);
    else
      pos += snprintf(buf + pos, size - pos, "%s", names[i]);
  }
  return 0;
}
