#include <stdlib.h>
#include <string.h>
#include "vfs.h"
#include "inode.h"
#include "dentry.h"

static char *fs_strdup(const char *s);
int fs_init(void);

static struct super_block g_sb; // global super block (metadata)

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

  // root inode
  struct inode *root_inode = malloc(sizeof(struct inode));
  if(!root_inode)
  {
    return -1;
  }
  memset(root_inode, 0, sizeof(*root_inode));
  root_inode->i_ino = 1;
  root_inode->i_type = FS_INODE_DIR;

  // root dentry
  struct dentry *root_dentry = malloc(sizeof(struct dentry));
  if(!root_dentry)
  {
    return -1;
  }
  memset(root_dentry, 0, sizeof(*root_dentry));
  root_dentry->d_name = fs_strdup("/");
  root_dentry->d_parent = root_dentry; // root parent to itself
  root_dentry->d_inode = root_inode;

  g_sb.s_root = root_dentry;

  return 0;
}

struct super_block *fs_get_super(void) 
{
  return &g_sb;
}

