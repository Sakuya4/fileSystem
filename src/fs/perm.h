#ifndef _PERM_H_
#define _PERM_H_

#include "inode.h"
#include "types.h"

/* permission mask */
#define FS_R_OK  0x4
#define FS_W_OK  0x2
#define FS_X_OK  0x1

/* user / group */
typedef struct {
  const char *name;
  fs_uid_t uid;
  fs_gid_t gid;
  const char *password;
} user_entry_t;

/* uid / gid control */
fs_uid_t fs_get_uid(void);
fs_gid_t fs_get_gid(void);
void fs_set_uid(fs_uid_t uid);
void fs_set_gid(fs_gid_t gid);

const char *fs_uid_name(fs_uid_t uid);
const char *fs_gid_name(fs_gid_t gid);

/* auth */
const user_entry_t *fs_get_user_by_name(const char *name);
int fs_authenticate(const user_entry_t *user);

/* permission check */
int fs_perm_check(const struct inode *inode, int mask);

#endif
