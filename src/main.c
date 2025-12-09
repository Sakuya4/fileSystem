#include <stdio.h>
#include "fs/vfs.h"
#include "fs/dentry.h"

int main(void)
{
  if(fs_init() != 0) 
  {
    printf("fs_init failed\n");
    return 1;
  }
  struct super_block *sb = fs_get_super();
  if(sb && sb->s_root && sb->s_root->d_name) 
  {
    printf("FS mounted, root name: %s\n", sb->s_root->d_name);
  }
  else
  {
    printf("FS init but root invalid\n");
  }
  return 0;
}