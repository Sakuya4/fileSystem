/* standard library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
/* standard library done */

/* user define library */
#include "vfs.h"
#include "vfs_internal.h"
#include "inode.h"
#include "dentry.h"
#include "path.h"
#include "block.h"
#include "perm.h"
/* user define library done */

/* local helper */
static const char *host_basename(const char *p)
{
  const char *a = strrchr(p, '/');
  const char *b = strrchr(p, '\\'); /* windows path */
  const char *s = a;

  if (!s || (b && b > s))
  {
    s = b;
  }

  return s ? (s + 1) : p;
}

static void join_vfs_path(char out[256], const char *dir, const char *base)
{
  size_t n;

  out[0] = '\0';

  if (!dir || dir[0] == '\0')
  {
    snprintf(out, 256, "%s", base);
    return;
  }

  /* dir = "/" */
  if (strcmp(dir, "/") == 0)
  {
    snprintf(out, 256, "/%s", base);
    return;
  }

  n = strlen(dir);

  /* dir endswith "/" */
  if (dir[n - 1] == '/')
  {
    snprintf(out, 256, "%s%s", dir, base);
  }
  else
  {
    snprintf(out, 256, "%s/%s", dir, base);
  }
}

static int inode_free_blocks(struct inode *inode)
{
  if (!inode)
  {
    return -1;
  }

  for (int i = 0; i < DIRECT_BLOCKS; i++)
  {
    if (inode->i_block[i] >= 0)
    {
      block_free(inode->i_block[i]);
      inode->i_block[i] = -1;
    }
  }
  return 0;
}

static int inode_write_bytes(struct inode *inode, const uint8_t *data, size_t len)
{
  if (!inode || (!data && len > 0))
  {
    return -1;
  }

  if (fs_perm_check(inode, FS_W_OK) != 0)
  {
    return -1;
  }

  size_t need_blocks = (len + BLOCK_SIZE - 1) / BLOCK_SIZE;
  if (need_blocks > DIRECT_BLOCKS)
  {
    return -1;
  }

  /* pre-check free space */
  if (block_free_size() < need_blocks * (size_t)BLOCK_SIZE)
  {
    return -1;
  }

  /* clear old blocks */
  inode_free_blocks(inode);

  /* alloc + write */
  for (size_t i = 0; i < need_blocks; i++)
  {
    int blk = block_alloc();
    if (blk < 0)
    {
      inode_free_blocks(inode);
      return -1;
    }

    inode->i_block[i] = blk;

    size_t off = i * (size_t)BLOCK_SIZE;
    size_t remain = len - off;
    size_t wlen = remain > (size_t)BLOCK_SIZE ? (size_t)BLOCK_SIZE : remain;

    uint8_t buf[BLOCK_SIZE];
    memset(buf, 0, sizeof(buf));
    if (wlen > 0)
    {
      memcpy(buf, data + off, wlen);
    }

    if (block_write(blk, buf) != 0)
    {
      inode_free_blocks(inode);
      return -1;
    }
  }

  inode->i_size = len;
  inode->i_mtime = (uint64_t)time(NULL);

  return 0;
}

static int inode_read_to_file(const struct inode *inode, FILE *fp)
{
  if (!inode || !fp)
  {
    return -1;
  }

  if (fs_perm_check(inode, FS_R_OK) != 0)
  {
    return -1;
  }

  size_t remain = inode->i_size;

  for (int i = 0; i < DIRECT_BLOCKS && remain > 0; i++)
  {
    int blk = inode->i_block[i];
    if (blk < 0)
    {
      break;
    }

    uint8_t buf[BLOCK_SIZE];
    if (block_read(blk, buf) != 0)
    {
      return -1;
    }

    size_t n = remain > (size_t)BLOCK_SIZE ? (size_t)BLOCK_SIZE : remain;
    if (n > 0)
    {
      if (fwrite(buf, 1, n, fp) != n)
      {
        return -1;
      }
    }
    remain -= n;
  }

  return 0;
}
/* local helper done */

/* =========================================================
 *  vfs_import(host_path, vfs_path)
 *  host -> VFS
 * ========================================================= */
int vfs_import(const char *host_path, const char *vfs_path)
{
  /* argument check */
  if (!host_path || !vfs_path || host_path[0] == '\0' || vfs_path[0] == '\0')
  {
    return -1;
  }

  /* ---------- (1) open host file ---------- */
  FILE *fp = fopen(host_path, "rb");
  if (!fp)
  {
    /* host file not found or cannot open */
    return -1;
  }

  /* ---------- (2) get size ---------- */
  if (fseek(fp, 0, SEEK_END) != 0)
  {
    fclose(fp);
    return -1;
  }

  long fsz = ftell(fp);
  if (fsz < 0)
  {
    fclose(fp);
    return -1;
  }

  if (fseek(fp, 0, SEEK_SET) != 0)
  {
    fclose(fp);
    return -1;
  }

  size_t len = (size_t)fsz;
  size_t max_len = (size_t)DIRECT_BLOCKS * (size_t)BLOCK_SIZE;

  if (len > max_len)
  {
    fclose(fp);
    return -1;
  }

  /* ---------- (3) read all bytes ---------- */
  uint8_t *data = NULL;
  if (len > 0)
  {
    data = (uint8_t*)malloc(len);
    if (!data)
    {
      fclose(fp);
      return -1;
    }

    if (fread(data, 1, len, fp) != len)
    {
      free(data);
      fclose(fp);
      return -1;
    }
  }
  fclose(fp);

  /* ---------- (4) normalize vfs target path ---------- */
  char target[256];
  strncpy(target, vfs_path, sizeof(target) - 1);
  target[sizeof(target) - 1] = '\0';

  trim(target);
  remove_multiple_slashes(target);
  rstrip_slash(target);

  if (target[0] == '\0')
  {
    if (data) free(data);
    return -1;
  }

  /* ---------- (5) if vfs_path is a dir -> auto append basename ---------- */
  struct dentry *maybe_dir = vfs_lookup(target);
  if (maybe_dir && maybe_dir->d_inode && maybe_dir->d_inode->i_type == FS_INODE_DIR)
  {
    char joined[256];
    const char *base = host_basename(host_path);

    join_vfs_path(joined, target, base);

    strncpy(target, joined, sizeof(target) - 1);
    target[sizeof(target) - 1] = '\0';
  }

  /* also handle "." / ".." special cases (they resolve to dir) */
  if (strcmp(target, ".") == 0 || strcmp(target, "..") == 0)
  {
    char joined[256];
    const char *base = host_basename(host_path);
    join_vfs_path(joined, target, base);

    strncpy(target, joined, sizeof(target) - 1);
    target[sizeof(target) - 1] = '\0';
  }

  /* ---------- (6) find or create file ---------- */
  struct dentry *dent = vfs_lookup(target);

  if (!dent)
  {
    if (vfs_create_file(target) != 0)
    {
      if (data) free(data);
      return -1;
    }

    dent = vfs_lookup(target);
    if (!dent || !dent->d_inode)
    {
      if (data) free(data);
      return -1;
    }
  }

  if (!dent->d_inode || dent->d_inode->i_type != FS_INODE_FILE)
  {
    if (data) free(data);
    return -1;
  }

  /* ---------- (7) permission + space + write ---------- */
  int rc = 0;

  if (fs_perm_check(dent->d_inode, FS_W_OK) != 0)
  {
    rc = -1;
  }
  else
  {
    /* reuse your existing write helper (inode_write_bytes) if you have it */
    rc = inode_write_bytes(dent->d_inode, data, len);
  }

  if (data) free(data);
  return rc;
}

/* =========================================================
 *  vfs_export(vfs_path, host_path)
 *  VFS -> host
 * ========================================================= */
int vfs_export(const char *vfs_path, const char *host_path)
{
  if (!vfs_path || !host_path || vfs_path[0] == '\0' || host_path[0] == '\0')
  {
    return -1;
  }

  struct dentry *dent = vfs_lookup(vfs_path);
  if (!dent || !dent->d_inode)
  {
    return -1;
  }

  if (dent->d_inode->i_type != FS_INODE_FILE)
  {
    return -1;
  }

  if (fs_perm_check(dent->d_inode, FS_R_OK) != 0)
  {
    return -1;
  }

  FILE *fp = fopen(host_path, "wb");
  if (!fp)
  {
    return -1;
  }

  int rc = inode_read_to_file(dent->d_inode, fp);

  fclose(fp);
  return rc;
}
