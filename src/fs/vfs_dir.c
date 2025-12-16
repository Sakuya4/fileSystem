/* standard library */
#include <stdio.h>
#include <time.h>
/* standard library done*/

/* user define */
#include "vfs.h"
#include "vfs_internal.h"
#include "inode.h"
#include "dentry.h"
#include "perm.h"
/* user define done */

#define C_RESET  "\x1b[0m"
#define C_BLUE   "\x1b[34m"
#define C_GREEN  "\x1b[32m"

/* ---------- helpers ---------- */
static const char *uid_to_name(fs_uid_t uid)
{
  if (uid == 0)
  {
    return "root";
  }
  return "user";
}

static int vfs_ls_dentry(struct dentry *dir)
{
  struct dentry *cur;
  if (!dir) return -1;

  for (cur = dir->d_child; cur != NULL; cur = cur->d_sibling)
  {
    if (cur->d_name)
      printf("%s\n", cur->d_name);
  }
  return 0;
}

static void fs_mode_to_str(fs_mode_t mode, char out[11])
{
  out[0] = (mode & FS_IFDIR) ? 'd' : '-';

  out[1] = (mode & FS_IRUSR) ? 'r' : '-';
  out[2] = (mode & FS_IWUSR) ? 'w' : '-';
  out[3] = (mode & FS_IXUSR) ? 'x' : '-';

  out[4] = (mode & FS_IRGRP) ? 'r' : '-';
  out[5] = (mode & FS_IWGRP) ? 'w' : '-';
  out[6] = (mode & FS_IXGRP) ? 'x' : '-';

  out[7] = (mode & FS_IROTH) ? 'r' : '-';
  out[8] = (mode & FS_IWOTH) ? 'w' : '-';
  out[9] = (mode & FS_IXOTH) ? 'x' : '-';

  out[10] = '\0';
}

static int vfs_ls_long_dentry(struct dentry *dir)
{
  struct dentry *cur;
  if (!dir) return -1;

  for (cur = dir->d_child; cur != NULL; cur = cur->d_sibling)
  {
    struct inode *inode = cur->d_inode;
    char mode_str[11] = "----------";
    char time_str[32] = "";
    time_t t;
    struct tm *tm;

    if (!inode) continue;

    fs_mode_to_str(inode->i_mode, mode_str);

    t  = (time_t)inode->i_mtime;
    tm = localtime(&t);
    if (tm)
      strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm);
    
    printf("%s %2u %-4s %4u %8lld %s %s\n",
           mode_str,
           (unsigned)inode->i_nlink,
           uid_to_name(inode->i_uid),
           (unsigned)inode->i_gid,
           (long long)inode->i_size,
           time_str,
           cur->d_name ? cur->d_name : "?");
  }
  return 0;
}

/* ---------- public API ---------- */

int vfs_ls(void)
{
  return vfs_ls_dentry(fs_get_cwd_dentry());
}

int vfs_ls_path(const char *path)
{
  struct dentry *target;

  if (!path || path[0] == '\0') return -1;

  target = vfs_lookup(path);
  if (!target || !target->d_inode) return -1;
  if (target->d_inode->i_type != FS_INODE_DIR) return -1;

  return vfs_ls_dentry(target);
}

int vfs_ls_long(void)
{
  struct dentry *cwd = fs_get_cwd_dentry();
  if (!cwd || !cwd->d_inode)
  {
    return -1;
  }

  if (fs_perm_check(cwd->d_inode, FS_R_OK) != 0)
  {
    return -1;
  }
  return vfs_ls_long_dentry(cwd);
}


int vfs_ls_long_path(const char *path)
{
  struct dentry *target;

  if (!path || path[0] == '\0')
  {
    return -1;
  }

  target = vfs_lookup(path);
  if (!target || !target->d_inode)
  {
    return -1;
  }

  if (target->d_inode->i_type != FS_INODE_DIR)
  {
    return -1;
  }

  if (fs_perm_check(target->d_inode, FS_R_OK) != 0)
  {
    return -1;
  }

  return vfs_ls_long_dentry(target);
}
