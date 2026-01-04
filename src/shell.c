/* standard library */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* standard library done */

/* user define */
#include "fs/vfs.h"
#include "fs/dentry.h"
#include "fs/path.h"
#include "fs/perm.h"
#include "fs/block.h" 
/* user define done */

/* marco */
#define SUDO_RESTORE(_is_sudo, _old_uid, _old_gid) \
  do { \
    if ((_is_sudo)) { \
      fs_set_uid((_old_uid)); \
      fs_set_gid((_old_gid)); \
      (_is_sudo) = 0; \
    } \
  } while (0)

/* marco done */

/* define function */
static void print_help(void);
void run_shell(void);
/* define function done*/

/* define */
#define CMD_BUF 256
/* define done */


static void print_help(void)
{
  printf("Commands:\n");
  printf("  help\n");
  printf("  exit\n");
  printf("  df\n");
  printf("  id\n");
  printf("  sudo <cmd>\n");
  printf("  ls [path]\n");
  printf("  tree [path]\n");
  printf("  cd <path>\n");
  printf("  mkdir <path>\n");
  printf("  rmdir <path>\n");
  printf("  touch <path>\n");
  printf("  stat <path>\n");
  printf("  cp <src> <dest>\n");
  printf("  write <path> <text>\n");
  printf("  vim <path> <text>\n");
  printf("  cat <path>\n");
  printf("  rm <path>\n");
  printf("  chmod <mode(octal)> <path>\n");
  printf("  import <host_path> <vfs_path>\n");
  printf("  export <vfs_path> <host_path>\n");
  printf("\nExamples:\n");
  printf("  mkdir a\n");
  printf("  touch a/x\n");
  printf("  write a/x hello\n");
  printf("  cat a/x\n");
  printf("  chmod 555 a\n");
  printf("  sudo rmdir a\n");
}

void run_shell(void)
{ 
  int is_sudo=0;
  fs_uid_t old_uid = fs_get_uid();
  fs_gid_t old_gid = fs_get_gid();

  char buf[CMD_BUF];
  printf("Total=%zu Used=%zu Free=%zu\n", block_total_size(), block_used_size(), block_free_size());
  while (1)
  {
    char cwd[256];

    if (vfs_get_cwd(cwd, sizeof(cwd)) != 0)
    {
      snprintf(cwd, sizeof(cwd), "?");
    }
    // printf("%s> ", cwd);
    const char *user = (fs_get_uid() == 0) ? "root" : "user";
    const char *prompt_char = (fs_get_uid() == 0) ? "#" : "$";
    printf("\x1b[32m%s@vfs\x1b[0m:"
       "\x1b[34m%s\x1b[0m"
       "%s ",
       user,
       cwd,
       prompt_char);

    if (!fgets(buf, sizeof(buf), stdin))
    {
      continue;
    }

    buf[strcspn(buf, "\n")] = '\0';
    trim(buf);
    if (strlen(buf) == 0)
    {
      continue;
    }
    /* sudo */
    
   /* sudo <command> */
    if (strncmp(buf, "sudo ", 5) == 0)
    {
      char *cmd = buf + 5;
      while (*cmd == ' ') cmd++;

      const user_entry_t *root = fs_get_user_by_name("root");
      if (!root || fs_authenticate(root) != 0) {
        printf("Authentication failed\n");
        continue;
      }

      old_uid = fs_get_uid();
      old_gid = fs_get_gid();
      is_sudo = 1; // 修正：原本邏輯應將標記設為 1

      fs_set_uid(0);
      fs_set_gid(0);

      memmove(buf, cmd, strlen(cmd) + 1);
    }

    /* su */
    if (strncmp(buf, "su", 2) == 0)
    {
      char *target = buf + 2;
      while (*target == ' ') target++;

      if (*target == '\0')
        target = "root";

      const user_entry_t *u = fs_get_user_by_name(target);
      if (!u || fs_authenticate(u) != 0) {
        printf("Authentication failed\n");
        continue;
      }

      fs_set_uid(u->uid);
      fs_set_gid(u->gid);
      printf("switched to %s\n", u->name);
      continue;
    }


    /* exit */
    if(strcmp(buf, "exit") == 0)
    {
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
      printf("%s", "Bye\n");
      break;
    }
    if (strcmp(buf, "help") == 0)
    {
      print_help();
      continue;
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
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
      
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
      continue;
    }
    /* df */
    if (strcmp(buf, "df")==0)
    {
      printf("Total=%zu Used=%zu Free=%zu\n", block_total_size(), block_used_size(), block_free_size());
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
      continue;
    }
    /* chmod <mode> <path> */
    if (strncmp(buf, "chmod ", 6) == 0)
    {
      char *arg = buf + 6;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      if (*arg == '\0')
      {
        printf("chmod: mode required\n");
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
        continue;
      }

      char *mode_str = arg;

      while (*arg && *arg != ' ' && *arg != '\t')
      {
        arg++;
      }

      if (*arg == '\0')
      {
        printf("chmod: path required\n");
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
        continue;
      }

      *arg = '\0';
      arg++;

      while (*arg == ' ' || *arg == '\t')
      {
        arg++;
      }

      char *path = arg;

      if (*path == '\0')
      {
        printf("chmod: path required\n");
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
        continue;
      }

      int mode = (int)strtol(mode_str, NULL, 8);

      if (vfs_chmod(path, mode) == 0)
      {
        printf("chmod ok: %s\n", path);
      }
      else
      {
        printf("chmod failed: %s\n", path);
      }
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
      continue;
    }
    /* check user */
    if (strcmp(buf, "id") == 0)
    {
      printf("uid=%d\n", (int)fs_get_uid());
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
      continue;
    }

    /* tree [path] */
    if (strncmp(buf, "tree", 4) == 0)
    {
      char pathbuf[256];
      const char *arg = buf + 4;
      while (*arg == ' ' || *arg == '\t') arg++;

      if (*arg == '\0') {
        vfs_tree(".");
      } else {
        strncpy(pathbuf, arg, sizeof(pathbuf) - 1);
        pathbuf[sizeof(pathbuf) - 1] = '\0';
        trim(pathbuf);
        remove_multiple_slashes(pathbuf);
        rstrip_slash(pathbuf);
        vfs_tree(pathbuf);
      }
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
      continue;
    }

    /* stat <path> */
    if (strncmp(buf, "stat ", 5) == 0)
    {
      char pathbuf[256];
      const char *arg = buf + 5;
      while (*arg == ' ' || *arg == '\t') arg++;

      strncpy(pathbuf, arg, sizeof(pathbuf) - 1);
      pathbuf[sizeof(pathbuf) - 1] = '\0';

      trim(pathbuf);
      remove_multiple_slashes(pathbuf);
      rstrip_slash(pathbuf);

      if (pathbuf[0] == '\0') {
        printf("stat: path required\n");
      } else {
        vfs_stat(pathbuf);
      }
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
      continue;
    }

    /* cp <src> <dest> */
    if (strncmp(buf, "cp ", 3) == 0)
    {
      char *arg = buf + 3;
      while (*arg == ' ' || *arg == '\t') arg++;
      char *src = arg;
      while (*arg && *arg != ' ' && *arg != '\t') arg++;

      if (*arg != '\0') {
        *arg = '\0';
        arg++;
      }

      while (*arg == ' ' || *arg == '\t') arg++;
      char *dest = arg;

      if (*src == '\0' || *dest == '\0') {
        printf("cp: source and destination required\n");
      } else {
        if (vfs_cp(src, dest) == 0) printf("cp ok\n");
        else printf("cp failed\n");
      }
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
        continue;
      }

      if (vfs_cat(pathbuf) != 0)
      {
        printf("cat failed: %s\n", pathbuf);
      }
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
        continue;
      }

      if (vfs_rmdir(pathbuf) == 0)
      {
        printf("rmdir ok: %s\n", pathbuf);
      }
      else
      {
        printf("rmdi  r failed: %s\n", pathbuf);
      }
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
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
      SUDO_RESTORE(is_sudo, old_uid, old_gid);
      continue;
    }
    
    /* import <host_path> <vfs_path> */
    if (strncmp(buf, "import ", 7) == 0)
    {
      char *arg = buf + 7;
      while (*arg == ' ' || *arg == '\t') arg++;

      if (*arg == '\0')
      {
        printf("import: host_path required\n");
        continue;
      }

      char *host = arg;
      while (*arg && *arg != ' ' && *arg != '\t') arg++;

      if (*arg == '\0')
      {
        printf("import: vfs_path required\n");
        continue;
      }

      *arg = '\0';
      arg++;
      while (*arg == ' ' || *arg == '\t') arg++;

      char *vpath = arg;
      if (*vpath == '\0')
      {
        printf("import: vfs_path required\n");
        continue;
      }

      if (vfs_import(host, vpath) == 0)
      {
        printf("import ok: %s -> %s\n", host, vpath);
      }
      else
      {
        printf("import failed: %s -> %s\n", host, vpath);
      }
      continue;
    }

    /* export <vfs_path> <host_path> */
    if (strncmp(buf, "export ", 7) == 0)
    {
      char *arg = buf + 7;
      while (*arg == ' ' || *arg == '\t') arg++;

      if (*arg == '\0')
      {
        printf("export: vfs_path required\n");
        continue;
      }

      char *vpath = arg;
      while (*arg && *arg != ' ' && *arg != '\t') arg++;

      if (*arg == '\0')
      {
        printf("export: host_path required\n");
        continue;
      }

      *arg = '\0';
      arg++;
      while (*arg == ' ' || *arg == '\t') arg++;

      char *host = arg;
      if (*host == '\0')
      {
        printf("export: host_path required\n");
        continue;
      }

      if (vfs_export(vpath, host) == 0)
      {
        printf("export ok: %s -> %s\n", vpath, host);
      }
      else
      {
        printf("export failed: %s -> %s\n", vpath, host);
      }
      continue;
    }


    /* vim <path> */
    if (strncmp(buf, "vim ", 4) == 0)
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
        printf("vim: path required\n");
        SUDO_RESTORE(is_sudo, old_uid, old_gid);
        continue;
      }

      if (vfs_vim(pathbuf) != 0)
      {
        printf("vim failed: %s\n", pathbuf);
      }

      SUDO_RESTORE(is_sudo, old_uid, old_gid);
      continue;
    }

    printf("Unknown command: %s\n", buf);
    SUDO_RESTORE(is_sudo, old_uid, old_gid); 
   }
}