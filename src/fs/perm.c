#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "perm.h"
#include "vfs_internal.h"
/* ---------- user database ---------- */
static user_entry_t g_users[] = {
  { "root", 0,    0,   "root" },
  { "user", 1000, 100, "user" },
};

#define USER_CNT (sizeof(g_users)/sizeof(g_users[0]))

/* ---------- current identity ---------- */
// static fs_uid_t g_uid = 1000;
// static fs_gid_t g_gid = 1000;


/* ---------- user lookup ---------- */
const user_entry_t *fs_get_user_by_name(const char *name)
{
  for (size_t i = 0; i < USER_CNT; i++)
    if (strcmp(g_users[i].name, name) == 0)
      return &g_users[i];
  return NULL;
}

/* ---------- authentication ---------- */
int fs_authenticate(const user_entry_t *user)
{
  char buf[64];

  if (!user)
    return -1;

  printf("Password: ");
  if (!fgets(buf, sizeof(buf), stdin))
    return -1;

  buf[strcspn(buf, "\n")] = '\0';

  if (strcmp(buf, user->password) == 0)
    return 0;

  return -1;
}

/* ---------- permission check ---------- */
int fs_perm_check(const struct inode *ino, int need)
{
  if (!ino)
    return -1;

  fs_uid_t uid = fs_get_uid();
  fs_gid_t gid = fs_get_gid();

  /* root bypass */
  if (uid == 0)
    return 0;

  int shift;

  if (uid == ino->i_uid)
    shift = 6;          /* owner */
  else if (gid == ino->i_gid)
    shift = 3;          /* group */
  else
    shift = 0;          /* other */

  int perm = (ino->i_mode >> shift) & 7;

  return ((perm & need) == need) ? 0 : -1;
}

const char *fs_uid_name(fs_uid_t uid)
{
  if (uid == 0)
  {
    return "root";
  }
  return "user";
}

const char *fs_gid_name(fs_gid_t gid)
{
  if (gid == 0)
  {
    return "root";
  }
  return "users";
}
