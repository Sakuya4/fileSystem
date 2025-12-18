/* standar library */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
/* standar library done */

/* user define library */
#include "vfs.h"
#include "vfs_internal.h"
#include "inode.h"
#include "dentry.h"
#include "path.h"
#include "block.h"
#include "perm.h"
/* user define library done */

/* user define function */
int vfs_cat(const char *path);
int vfs_create_file(const char *path);
int vfs_write_all(const char *path, const char *data);
/* user define function done */

/* function */

int vfs_cat(const char *path)
{
    struct dentry *dent;
    struct inode  *inode;

    if (!path) return -1;

    dent = vfs_lookup(path);
    if (!dent || !dent->d_inode)
    {
      return -1;
    }
    inode = dent->d_inode;
    if (inode->i_type != FS_INODE_FILE)
    {
      return -1;
    }
    if(fs_perm_check(inode, FS_R_OK)!= 0)
    {
      return -1;
    }
    size_t remain = inode->i_size;
    for (int i = 0; i < DIRECT_BLOCKS && remain > 0; i++) {
        int blk = inode->i_block[i];
        if (blk < 0) break;

        uint8_t buf[BLOCK_SIZE];
        if (block_read(blk, buf) != 0) return -1;

        size_t n = (remain > BLOCK_SIZE) ? BLOCK_SIZE : remain;
        fwrite(buf, 1, n, stdout);
        remain -= n;
    }
    printf("\n");
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
  {
    return -1;
  }
  if (!parent || !parent->d_inode)
  {
    return -1;
  }
  if(parent->d_inode->i_type != FS_INODE_DIR)
  {  
    return -1;
  }
  if(fs_perm_check(parent->d_inode, FS_W_OK | FS_X_OK) != 0)
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
  inode->i_ino   = 0;
  inode->i_type  = FS_INODE_FILE;
  inode->i_mode  = FS_IFREG | 0644;
  inode->i_uid   = fs_get_uid();
  inode->i_gid   = fs_get_gid();
  inode->i_nlink = 1;
  inode->i_size  = 0;
  inode->i_mtime = (uint64_t)time(NULL);

  for (int i = 0; i < DIRECT_BLOCKS; i++)
  {
    inode->i_block[i] = -1;
  }

 
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

int vfs_write_all(const char *path, const char *data)
{
    struct dentry *dent;
    struct inode  *inode;
    size_t len;
    size_t need_blocks;
    size_t i, j;

    if (!path || !data)
        return -1;

    dent = vfs_lookup(path);
    if (!dent || !dent->d_inode)
        return -1;

    inode = dent->d_inode;

    if (inode->i_type != FS_INODE_FILE)
    {
      return -1;
    }
    if(fs_perm_check(inode, FS_W_OK) != 0)
    {
      return -1;
    }

    len = strlen(data);

    /* ---------- (2) 先檢查大小 ---------- */
    need_blocks = (len + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (need_blocks > DIRECT_BLOCKS)
        return -1;  /* file too large */

    /* ---------- 釋放舊 blocks ---------- */
    for (i = 0; i < DIRECT_BLOCKS; i++) {
        if (inode->i_block[i] >= 0) {
            block_free(inode->i_block[i]);
            inode->i_block[i] = -1;
        }
    }

    /* ---------- alloc + write ---------- */
    for (i = 0; i < need_blocks; i++) {
        int blk = block_alloc();
        if (blk < 0) {
            /* rollback */
            for (j = 0; j < i; j++) {
                block_free(inode->i_block[j]);
                inode->i_block[j] = -1;
            }
            return -1;
        }

        inode->i_block[i] = blk;

        size_t offset = i * BLOCK_SIZE;
        size_t remain = len - offset;
        size_t write_size = remain > BLOCK_SIZE ? BLOCK_SIZE : remain;

        uint8_t buf[BLOCK_SIZE];
        memset(buf, 0, BLOCK_SIZE);
        memcpy(buf, data + offset, write_size);

        /* ---------- (3) block_write 失敗檢查 ---------- */
        if (block_write(blk, buf) != 0) {
            /* rollback */
            for (j = 0; j <= i; j++) {
                if (inode->i_block[j] >= 0) {
                    block_free(inode->i_block[j]);
                    inode->i_block[j] = -1;
                }
            }
            return -1;
        }
    }
    /* ---------- 更新 inode ---------- */
    inode->i_size  = len;
    inode->i_mtime = (uint64_t)time(NULL);

    return 0;
}

/* function done */
