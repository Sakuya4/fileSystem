/* standard library */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* standard library done */

/* define */
#include "vfs.h"
#include "inode.h"
#include "dentry.h"
/* define done */

/* define function */
struct super_block *fs_get_super(void);
static char *fs_strdup(const char *s);
int fs_init(void);
static int dentry_add_child(struct dentry *parent, struct dentry *child);
static struct dentry *dentry_find_child(struct dentry *parent, const char *name);
int vfs_mkdir(const char *name);
int vfs_ls(void);
int vfs_cd(const char *name);
/* define function done */

static struct super_block g_sb; // global super block (metadata)
static struct dentry *g_cwd; // current working directory

struct super_block *fs_get_super(void)
{
  return &g_sb;
}

static char *fs_strdup(const char *s)
{
  size_t len = strlen(s) + 1;
  char *p = malloc(len);
  if (p)
  {
    memcpy(p, s, len);
  }
  return p;
}

int fs_init(void)
{
  memset(&g_sb, 0, sizeof(g_sb));
  g_sb.s_magic = 0x12345678;

  /* root inode */
  struct inode *root_inode = malloc(sizeof(struct inode));
  if(!root_inode)
  {
    return -1;
  }

  memset(root_inode, 0, sizeof(*root_inode));
  root_inode->i_ino  = 1;
  root_inode->i_type = FS_INODE_DIR;

  /* root dentry */
  struct dentry *root_dentry = malloc(sizeof(struct dentry));
  if(!root_dentry)
  {
    free(root_inode);
    return -1;
  }

  memset(root_dentry, 0, sizeof(*root_dentry));
  root_dentry->d_name   = fs_strdup("/");
  root_dentry->d_parent = root_dentry; /* root parent to it self */
  root_dentry->d_inode  = root_inode;

  g_sb.s_root = root_dentry;
  g_cwd       = root_dentry;  
  return 0;
}

static int dentry_add_child(struct dentry *parent, struct dentry *child)
{
  if(parent == NULL || child == NULL)
  {
    return -1;
  }

  child->d_parent  = parent;
  child->d_sibling = parent->d_child;  /* into front */
  parent->d_child  = child;

  return 0;
}


static struct dentry *dentry_find_child(struct dentry *parent, const char *name)
{
  struct dentry *cur;

  if(parent == NULL || name == NULL)
  {
    return NULL;
  }

  for(cur = parent->d_child; cur != NULL; cur = cur->d_sibling)
  {
    if(cur->d_name != NULL && strcmp(cur->d_name, name) == 0)
    {
      return cur;
    }
  }

  return NULL;
}

int vfs_mkdir(const char *name)
{
  struct inode  *inode;
  struct dentry *dentry;

  if(name == NULL || name[0] == '\0')
  {
    return -1;
  }

  /* Phase 2: 先不支援有 '/' 的路徑，只允許單一名稱 */
  if(strchr(name, '/') != NULL)
  {
    return -1;
  }

  /* 同名已存在就失敗 */
  if(dentry_find_child(g_cwd, name) != NULL)
  {
    return -1;
  }

  inode = malloc(sizeof(struct inode));
  if(inode == NULL)
  {
    return -1;
  }

  memset(inode, 0, sizeof(*inode));
  inode->i_ino  = 0;              /* TODO: 之後做真正的 inode allocator */
  inode->i_type = FS_INODE_DIR;

  dentry = malloc(sizeof(struct dentry));
  if(dentry == NULL)
  {
    free(inode);
    return -1;
  }

  memset(dentry, 0, sizeof(*dentry));
  dentry->d_name = fs_strdup(name);
  if(dentry->d_name == NULL)
  {
    free(dentry);
    free(inode);
    return -1;
  }

  dentry->d_inode = inode;

  if(dentry_add_child(g_cwd, dentry) != 0)
  {
    free(dentry->d_name);
    free(dentry);
    free(inode);
    return -1;
  }

  return 0;
}

int vfs_ls(void)
{
  struct dentry *cur;

  if(g_cwd == NULL)
  {
    return -1;
  }

  for(cur = g_cwd->d_child; cur != NULL; cur = cur->d_sibling)
  {
    if(cur->d_name != NULL)
    {
      printf("%s\n", cur->d_name);
    }
  }

  return 0;
}

int vfs_cd(const char *name)
{
  struct dentry *target;

  if(name == NULL || name[0] == '\0')
  {
    return -1;
  }

  if(strcmp(name, ".") == 0)
  {
    return 0;  /* 不變 */
  }

  if(strcmp(name, "..") == 0)
  {
    if(g_cwd != NULL && g_cwd->d_parent != NULL)
    {
      g_cwd = g_cwd->d_parent;
      return 0;
    }
    return -1;
  }

  /* 其他情況：找 child 目錄 */
  target = dentry_find_child(g_cwd, name);
  if(target == NULL || target->d_inode == NULL)
  {
    return -1;
  }

  if(target->d_inode->i_type != FS_INODE_DIR)
  {
    return -1;  /* 不是目錄，不能 cd */
  }

  g_cwd = target;
  return 0;
}