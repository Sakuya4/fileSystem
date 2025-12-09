#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fs/vfs.h"
#include "fs/dentry.h"

#define CMD_BUF 256

static void run_shell(void)
{
  char buf[CMD_BUF];

  while(1)
  {
    printf("fs> ");          /* 顯示 prompt */
    
    if(!fgets(buf, sizeof(buf), stdin))
    {
      continue;
    }

    /* 移除換行字元 */
    buf[strcspn(buf, "\n")] = '\0';

    /* 空指令 */
    if(strlen(buf) == 0)
    {
      continue;
    }

    /* exit */
    if(strcmp(buf, "exit") == 0)
    {
      printf("Bye!\n");
      break;
    }

    /* mkdir NAME */
    if(strncmp(buf, "mkdir ", 6) == 0)
    {
      const char *name = buf + 6;
      if(vfs_mkdir(name) == 0)
      {
        printf("mkdir ok: %s\n", name);
      }
      else
      {
        printf("mkdir failed: %s\n", name);
      }
      continue;
    }

    /* cd NAME */
    if(strncmp(buf, "cd ", 3) == 0)
    {
      const char *name = buf + 3;
      if(vfs_cd(name) == 0)
      {
        printf("cd ok: %s\n", name);
      }
      else
      {
        printf("cd failed: %s\n", name);
      }
      continue;
    }

    /* ls */
    if(strcmp(buf, "ls") == 0)
    {
      vfs_ls();
      continue;
    }

    printf("Unknown command: %s\n", buf);
  }
}

int main(void)
{
  if(fs_init() != 0)
  {
    printf("fs_init failed\n");
    return 1;
  }

  printf("Filesystem mounted. Type commands.\n");
  printf("Available: mkdir <name>, cd <name>, ls, exit\n");
  printf("----------------------------------------------\n");

  run_shell();
  return 0;
}
