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
    char cwd[256];

    if(vfs_get_cwd(cwd, sizeof(cwd)) != 0)
    {
      snprintf(cwd, sizeof(cwd), "?");
    }
    printf("%s> ", cwd);

    if(!fgets(buf, sizeof(buf), stdin))
    {
      continue;
    }

    buf[strcspn(buf, "\n")] = '\0';

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

    /* mkdir <path> */
    if(strncmp(buf, "mkdir ", 6) == 0)
    {
      const char *path = buf + 6;

      while(*path == ' ' || *path == '\t')
      {
        path++;
      }

      if(*path == '\0')
      {
        printf("mkdir: path required\n");
        continue;
      }

      if(vfs_mkdir(path) == 0)
      {
        printf("mkdir ok: %s\n", path);
      }
      else
      {
        printf("mkdir failed: %s\n", path);
      }
      continue;
    }

    /* cd <path> */
    if(strncmp(buf, "cd ", 3) == 0)
    {
      const char *path = buf + 3;

      while(*path == ' ' || *path == '\t')
      {
        path++;
      }

      if(*path == '\0')
      {
        printf("cd: path required\n");
        continue;
      }

      if(vfs_cd(path) == 0)
      {
        printf("cd ok: %s\n", path);
      }
      else
      {
        printf("cd failed: %s\n", path);
      }
      continue;
    }

    /* touch <path> */
    if(strncmp(buf, "touch ", 6) == 0)
    {
      const char *path = buf + 6;

      while(*path == ' ' || *path == '\t')
      {
        path++;
      }

      if(*path == '\0')
      {
        printf("touch: path required\n");
        continue;
      }

      if(vfs_create_file(path) == 0)
      {
        printf("touch ok: %s\n", path);
      }
      else
      {
        printf("touch failed: %s\n", path);
      }
      continue;
    }

    /* write <path> <text...> */
    if(strncmp(buf, "write ", 6) == 0)
    {
      char *arg  = buf + 6;
      char *path;
      char *data;

      /* skip spaces before path */
      while(*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      if(*arg == '\0')
      {
        printf("write: path required\n");
        continue;
      }

      path = arg;

      /* 找 path 和 data 的分隔空白 */
      while(*arg && *arg != ' ' && *arg != '\t')
      {
        arg++;
      }

      if(*arg == '\0')
      {
        printf("write: data required\n");
        continue;
      }

      *arg = '\0'; /* 把 path 字串結尾切斷 */
      arg++;

      while(*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      data = arg;

      if(vfs_write_all(path, data) == 0)
      {
        printf("write ok: %s\n", path);
      }
      else
      {
        printf("write failed: %s\n", path);
      }
      continue;
    }

    /* cat <path> */
    if(strncmp(buf, "cat ", 4) == 0)
    {
      const char *path = buf + 4;

      while(*path == ' ' || *path == '\t')
      {
        path++;
      }

      if(*path == '\0')
      {
        printf("cat: path required\n");
        continue;
      }

      if(vfs_cat(path) != 0)
      {
        printf("cat failed: %s\n", path);
      }
      continue;
    }
    /* rm <path> */
    if(strncmp(buf, "rm ", 3) == 0)
    {
      const char *path = buf + 3;

      while(*path == ' ' || *path == '\t')
      {
        path++;
      }

      if(*path == '\0')
      {
        printf("rm: path required\n");
        continue;
      }

      if(vfs_rm(path) == 0)
      {
        printf("rm ok: %s\n", path);
      }
      else
      {
        printf("rm failed: %s\n", path);
      }
      continue;
    }

    /* rmdir <path> */
    if(strncmp(buf, "rmdir ", 6) == 0)
    {
      const char *path = buf + 6;

      while(*path == ' ' || *path == '\t')
      {
        path++;
      }

      if(*path == '\0')
      {
        printf("rmdir: path required\n");
        continue;
      }

      if(vfs_rmdir(path) == 0)
      {
        printf("rmdir ok: %s\n", path);
      }
      else
      {
        printf("rmdir failed: %s\n", path);
      }
      continue;
    }

    /* ls 或 ls <path> */
        /* ls -l 或 ls -l <path> */
   if (strncmp(buf, "ls -l", 5) == 0)
    {
        const char *arg = buf + 5;

        while (*arg == ' ' || *arg == '\t')
            arg++;

        if (*arg == '\0')
        {
            vfs_ls_long();
        }
        else
        {
            if (vfs_ls_long_path(arg) != 0)
                printf("ls -l failed: %s\n", arg);
        }
        continue;
    }

    /* ls → 等同 ls -l */
    if (strcmp(buf, "ls") == 0)
    {
        vfs_ls_long();
        continue;
    }

    /* ls <path> → 等同 ls -l <path> */
    if (strncmp(buf, "ls ", 3) == 0)
    {
        const char *arg = buf + 3;

        while (*arg == ' ' || *arg == '\t')
        {
          arg++;
        }
        if (*arg == '\0')
        {
          vfs_ls_long();
        }
        else
        {
          if (vfs_ls_long_path(arg) != 0)
          printf("ls failed: %s\n", arg);
        }
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
  printf("Available: mkdir, cd, ls [path], ls -l [path], touch, write, cat, rm, rmdir, exit\n");
  printf("touch <path>, write <path> <text>, cat <path>, rm <path>, rmdir <path>, exit\n");
  printf("----------------------------------------------\n");

  run_shell();
  return 0;
}
