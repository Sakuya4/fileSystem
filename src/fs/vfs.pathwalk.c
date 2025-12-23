#include <string.h>
/* user define */
#include "vfs.h"
#include "vfs_internal.h"
#include "path.h"
#include "dentry.h"
#include "inode.h"
/* user define end */

path_normalize(buf, sizeof(buf), path);
tok=path_next_token(&p);

struct dentry *vfs_lookup(const char *path)
{
  char buf[256];
  struct dentry *cur;
  char *p;
  char *tok;

  if (path == NULL || *path == '\0')
  {
    return NULL;
  }
  /* copy & normalize */
  path_normalize(buf, sizeof(buf), path);
  if (buf[0] == '\0')
  {
    return NULL;
  }

  if (buf[0] == '/')
  {
    cur = fs_get_super()->s_root;
    if (buf[1] == '\0')
      return cur;

    p = buf + 1;
  }
  else
  {
    cur = fs_get_cwd_dentry();
    p = buf;
  }

  while ((tok = path_next_token(&p)) != NULL)
  {
    if (tok[0] == '\0')
      continue;

    if (strcmp(tok, ".") == 0)
      continue;

    if (strcmp(tok, "..") == 0)
    {
      if (cur && cur->d_parent)
        cur = cur->d_parent;
      continue;
    }

    cur = dentry_find_child(cur, tok);
    if (cur == NULL)
      return NULL;
  }

  return cur;
}