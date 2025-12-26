/* standard library */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
/* standard library done */

/* user define */
#include "vfs.h"
#include "vfs_internal.h"
#include "inode.h"
#include "dentry.h"
#include "block.h"
#include "perm.h"
#include "path.h"
/* user define done */

#define VIM_MAX_BUF 4096
#define VIM_LINE_BUF 256

static int vfs_read_all_into_buf(const char *path, char *out, size_t out_sz)
{
  struct dentry *dent;
  struct inode *inode;

  if (!path || !out || out_sz == 0)
  {
    return -1;
  }

  out[0] = '\0';

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

  if (fs_perm_check(inode, FS_R_OK) != 0)
  {
    return -1;
  }

  size_t remain = inode->i_size;
  size_t pos = 0;

  for (int i = 0; i < DIRECT_BLOCKS && remain > 0; i++)
  {
    int blk = inode->i_block[i];
    if (blk < 0)
    {
      break;
    }

    uint8_t b[BLOCK_SIZE];
    if (block_read(blk, b) != 0)
    {
      return -1;
    }

    size_t n = (remain > BLOCK_SIZE) ? BLOCK_SIZE : remain;

    if (pos + n >= out_sz)
    {
      n = (out_sz - 1) - pos;
    }

    memcpy(out + pos, b, n);
    pos += n;
    out[pos] = '\0';

    remain -= n;

    if (pos >= out_sz - 1)
    {
      break;
    }
  }

  out[out_sz - 1] = '\0';
  return 0;
}

static void vim_print_help(void)
{
  printf("mini-vim commands:\n");
  printf("  :p    print buffer\n");
  printf("  :c    clear buffer\n");
  printf("  :w    write(save)\n");
  printf("  :q    quit\n");
  printf("  :wq   write and quit\n");
  printf("  (any other line will be appended)\n");
}

int vfs_vim(const char *path)
{
  char buf[VIM_MAX_BUF];
  char line[VIM_LINE_BUF];
  int modified = 0;

  if (!path)
  {
    return -1;
  }

  /* normalize path like your shell does (optional but safer) */
  char pathbuf[256];
  strncpy(pathbuf, path, sizeof(pathbuf) - 1);
  pathbuf[sizeof(pathbuf) - 1] = '\0';
  trim(pathbuf);
  remove_multiple_slashes(pathbuf);
  rstrip_slash(pathbuf);

  if (pathbuf[0] == '\0')
  {
    printf("vim: path required\n");
    return -1;
  }

  /* if file not exist -> create (need parent W|X) */
  struct dentry *dent = vfs_lookup(pathbuf);
  if (!dent)
  {
    if (vfs_create_file(pathbuf) != 0)
    {
      printf("vim: create failed: %s\n", pathbuf);
      return -1;
    }
  }

  memset(buf, 0, sizeof(buf));

  /* read current content (if any) */
  dent = vfs_lookup(pathbuf);
  if (dent && dent->d_inode && dent->d_inode->i_type == FS_INODE_FILE)
  {
    /* if no read permission, deny */
    if (fs_perm_check(dent->d_inode, FS_R_OK) != 0)
    {
      printf("vim: permission denied (read): %s\n", pathbuf);
      return -1;
    }

    /* read into buf (ignore read fail -> empty) */
    (void)vfs_read_all_into_buf(pathbuf, buf, sizeof(buf));
  }

  printf("----- vim: %s -----\n", pathbuf);
  vim_print_help();
  printf("\n");
  printf("%s", buf);

  while (1)
  {
    printf("vim> ");

    if (!fgets(line, sizeof(line), stdin))
    {
      printf("\n");
      break;
    }

    line[strcspn(line, "\n")] = '\0';
    trim(line);

    if (strcmp(line, ":h") == 0 || strcmp(line, ":help") == 0)
    {
      vim_print_help();
      continue;
    }

    if (strcmp(line, ":p") == 0)
    {
      printf("%s", buf);
      continue;
    }

    if (strcmp(line, ":c") == 0)
    {
      buf[0] = '\0';
      modified = 1;
      printf("(cleared)\n");
      continue;
    }

    if (strcmp(line, ":q") == 0)
    {
      if (modified)
      {
        printf("warning: unsaved changes, use :w or :wq\n");
      }
      break;
    }

    if (strcmp(line, ":w") == 0)
    {
      dent = vfs_lookup(pathbuf);
      if (!dent || !dent->d_inode)
      {
        printf("vim: file disappeared?\n");
        continue;
      }
      if (fs_perm_check(dent->d_inode, FS_W_OK) != 0)
      {
        printf("vim: permission denied (write): %s\n", pathbuf);
        continue;
      }

      if (vfs_write_all(pathbuf, buf) == 0)
      {
        modified = 0;
        printf("written\n");
      }
      else
      {
        printf("write failed\n");
      }
      continue;
    }

    if (strcmp(line, ":wq") == 0)
    {
      dent = vfs_lookup(pathbuf);
      if (!dent || !dent->d_inode)
      {
        printf("vim: file disappeared?\n");
        break;
      }
      if (fs_perm_check(dent->d_inode, FS_W_OK) != 0)
      {
        printf("vim: permission denied (write): %s\n", pathbuf);
        continue;
      }

      if (vfs_write_all(pathbuf, buf) == 0)
      {
        printf("written\n");
      }
      else
      {
        printf("write failed\n");
      }
      break;
    }

    /* append line */
    if (line[0] != '\0')
    {
      size_t cur = strlen(buf);
      size_t add = strlen(line);

      if (cur + add + 2 >= sizeof(buf))
      {
        printf("vim: buffer full\n");
        continue;
      }

      memcpy(buf + cur, line, add);
      cur += add;
      buf[cur++] = '\n';
      buf[cur] = '\0';

      modified = 1;
    }
  }

  printf("----- leave vim -----\n");
  return 0;
}
