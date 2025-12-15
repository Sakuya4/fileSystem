#include <stdint.h>

#include "perm.h"
#include "vfs_internal.h"

int fs_perm_check(const struct inode *ino, int need)
{
  if (!ino)
  {
    return -1;
  }
  /* check sudo or not*/
  if (fs_get_uid() == 0)
  {
    return 0;
  }
  /* check sudo or not done */
  fs_uid_t uid = fs_get_uid();

  /* root bypass */
  if (uid == 0)
  {
    return 0;
  }
  int shift = 0;
  if (uid == ino->i_uid)
  {
    shift = 6;
  }
  else
  {
    shift = 0;
  }

  int perm = (ino->i_mode >> shift) & 7;

  if ((perm & need) == need)
  {
    return 0;
  }

  return -1;
}