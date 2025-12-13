/* standard library */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* standard library done */

/* user define */
#include "fs/vfs.h"
#include "fs/dentry.h"
#include "fs/path.h"

#include "fs/block.h" //test
/* user define done */

/* define function */
void run_shell(void);
/* define function done*/

/* define */
#define CMD_BUF 256
/* define done */

void run_shell(void)
{
  char buf[CMD_BUF];
  printf("Total=%zu Used=%zu Free=%zu\n", block_total_size(), block_used_size(), block_free_size());
  while (1)
  {
    char cwd[256];

    if (vfs_get_cwd(cwd, sizeof(cwd)) != 0)
    {
      snprintf(cwd, sizeof(cwd), "?");
    }
    printf("%s> ", cwd);

    if (!fgets(buf, sizeof(buf), stdin))
    {
      continue;
    }

    buf[strcspn(buf, "\n")] = '\0';

    if (strlen(buf) == 0)
    {
      continue;
    }

    /* exit */
    if (strcmp(buf, "exit") == 0)
    {
      printf("Bye!\n");
      break;
    }

    /* mkdir <path> */
    if (strncmp(buf, "mkdir ", 6) == 0)
    {
      char pathbuf[256];
      const char *arg = buf + 6;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      strncpy(pathbuf, arg, sizeof(pathbuf) - 1);
      pathbuf[sizeof(pathbuf) - 1] = '\0';

      trim(pathbuf);
      remove_multiple_slashes(pathbuf);
      rstrip_slash(pathbuf);

      if (pathbuf[0] == '\0')
      {
        printf("mkdir: path required\n");
        continue;
      }

      if (vfs_mkdir(pathbuf) == 0)
      {
        printf("mkdir ok: %s\n", pathbuf);
      }
      else
      {
        printf("mkdir failed: %s\n", pathbuf);
      }
      continue;
    }

    /* cd <path> */
    if (strncmp(buf, "cd ", 3) == 0)
    {
      char pathbuf[256];
      const char *arg = buf + 3;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      strncpy(pathbuf, arg, sizeof(pathbuf) - 1);
      pathbuf[sizeof(pathbuf) - 1] = '\0';

      trim(pathbuf);
      remove_multiple_slashes(pathbuf);
      rstrip_slash(pathbuf);

      if (pathbuf[0] == '\0')
      {
        printf("cd: path required\n");
        continue;
      }

      if (vfs_cd(pathbuf) == 0)
      {
        printf("cd ok: %s\n", pathbuf);
      }
      else
      {
        printf("cd failed: %s\n", pathbuf);
      }
      continue;
    }

    /* touch <path> */
    if (strncmp(buf, "touch ", 6) == 0)
    {
      char pathbuf[256];
      const char *arg = buf + 6;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      strncpy(pathbuf, arg, sizeof(pathbuf) - 1);
      pathbuf[sizeof(pathbuf) - 1] = '\0';

      trim(pathbuf);
      remove_multiple_slashes(pathbuf);
      rstrip_slash(pathbuf);

      if (pathbuf[0] == '\0')
      {
        printf("touch: path required\n");
        continue;
      }

      if (vfs_create_file(pathbuf) == 0)
      {
        printf("touch ok: %s\n", pathbuf);
      }
      else
      {
        printf("touch failed: %s\n", pathbuf);
      }
      continue;
    }

    /* write <path> <text...> */
    if (strncmp(buf, "write ", 6) == 0)
    {
      char *arg  = buf + 6;
      char pathbuf[256];
      char *path;
      char *data;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      if (*arg == '\0')
      {
        printf("write: path required\n");
        continue;
      }

      path = arg;

      /* 找 path 和 data 的分隔空白 */
      while (*arg && *arg != ' ' && *arg != '\t')
      {
        arg++;
      }

      if (*arg == '\0')
      {
        printf("write: data required\n");
        continue;
      }

      *arg = '\0';
      arg++;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      data = arg;

      /* 正規化 path */
      strncpy(pathbuf, path, sizeof(pathbuf) - 1);
      pathbuf[sizeof(pathbuf) - 1] = '\0';

      trim(pathbuf);
      remove_multiple_slashes(pathbuf);
      rstrip_slash(pathbuf);

      if (vfs_write_all(pathbuf, data) == 0)
      {
        printf("write ok: %s\n", pathbuf);
      }
      else
      {
        printf("write failed: %s\n", pathbuf);
      }
      continue;
    }

    /* cat <path> */
    if (strncmp(buf, "cat ", 4) == 0)
    {
      char pathbuf[256];
      const char *arg = buf + 4;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      strncpy(pathbuf, arg, sizeof(pathbuf) - 1);
      pathbuf[sizeof(pathbuf) - 1] = '\0';

      trim(pathbuf);
      remove_multiple_slashes(pathbuf);
      rstrip_slash(pathbuf);

      if (pathbuf[0] == '\0')
      {
        printf("cat: path required\n");
        continue;
      }

      if (vfs_cat(pathbuf) != 0)
      {
        printf("cat failed: %s\n", pathbuf);
      }
      continue;
    }

    /* rm <path> */
    if (strncmp(buf, "rm ", 3) == 0)
    {
      char pathbuf[256];
      const char *arg = buf + 3;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      strncpy(pathbuf, arg, sizeof(pathbuf) - 1);
      pathbuf[sizeof(pathbuf) - 1] = '\0';

      trim(pathbuf);
      remove_multiple_slashes(pathbuf);
      rstrip_slash(pathbuf);

      if (pathbuf[0] == '\0')
      {
        printf("rm: path required\n");
        continue;
      }

      if (vfs_rm(pathbuf) == 0)
      {
        printf("rm ok: %s\n", pathbuf);
      }
      else
      {
        printf("rm failed: %s\n", pathbuf);
      }
      continue;
    }

    /* rmdir <path> */
    if (strncmp(buf, "rmdir ", 6) == 0)
    {
      char pathbuf[256];
      const char *arg = buf + 6;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      strncpy(pathbuf, arg, sizeof(pathbuf) - 1);
      pathbuf[sizeof(pathbuf) - 1] = '\0';

      trim(pathbuf);
      remove_multiple_slashes(pathbuf);
      rstrip_slash(pathbuf);

      if (pathbuf[0] == '\0')
      {
        printf("rmdir: path required\n");
        continue;
      }

      if (vfs_rmdir(pathbuf) == 0)
      {
        printf("rmdir ok: %s\n", pathbuf);
      }
      else
      {
        printf("rmdir failed: %s\n", pathbuf);
      }
      continue;
    }

    /* ls 或 ls <path> → 直接走 long 格式 */
    if (strncmp(buf, "ls", 2) == 0)
    {
      const char *arg = buf + 2;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      if (*arg == '\0')
      {
        /* ls */
        vfs_ls_long();
      }
      else
      {
        /* ls <path> */
        if (vfs_ls_long_path(arg) != 0)
        {
          printf("ls failed: %s\n", arg);
        }
      }
      continue;
    }

    printf("Unknown command: %s\n", buf);
  }
}