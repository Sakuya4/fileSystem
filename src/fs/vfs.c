/* standard library */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* standard library done */

/* define */
#include "vfs.h"
#include "inode.h"
#include "dentry.h"
/* define done */

/* define function */
struct super_block *fs_get_super(void);
static char *fs_strdup(const char *s);
int fs_init(void);
static int dentry_add_child(struct dentry *parent, struct dentry *child);
static struct dentry *dentry_find_child(struct dentry *parent, const char *name);
static int vfs_mkdir_path_internal(const char *path);
int vfs_mkdir(const char *path);
static int vfs_ls_dentry(struct dentry *dir);
int vfs_ls(void);
int vfs_ls_path(const char *path);
int vfs_cd(const char *path);
int vfs_get_cwd(char *buf, size_t size);
static void trim(char *s);

int vfs_create_file(const char *path);
int vfs_write_all(const char *path, const char *data);
int vfs_cat(const char *path);

static int dentry_remove_child(struct dentry *parent, struct dentry *child);
int vfs_rm(const char *path);
int vfs_rmdir(const char *path);

static void fs_mode_to_str(fs_mode_t mode, char out[11]);
/* define function done */

static struct super_block g_sb; // global super block (metadata)
static struct dentry *g_cwd; // current working directory

static int vfs_ls_long_dentry(struct dentry *dir);
int vfs_ls_long(void);
int vfs_ls_long_path(const char *path);

/* Function define */
struct super_block *fs_get_super(void)
{
  return &g_sb;
}

static char *fs_strdup(const char *s)
{
  size_t len = strlen(s) + 1;
  char *p = malloc(len);
  if (p)
  {
    memcpy(p, s, len);
  }
  return p;
}

int fs_init(void)
{
  memset(&g_sb, 0, sizeof(g_sb));
  g_sb.s_magic = 0x12345678;

  /* root inode */
  struct inode *root_inode = malloc(sizeof(struct inode));
  if(!root_inode)
  {
    return -1;
  }

  memset(root_inode, 0, sizeof(*root_inode));
  root_inode->i_ino  = 1;
  root_inode->i_type = FS_INODE_DIR;
  
  root_inode->i_mode  = FS_IFDIR | FS_IRUSR | FS_IWUSR | FS_IXUSR |
                        FS_IRGRP | FS_IXGRP |
                        FS_IROTH | FS_IXOTH;  // like 0755
  root_inode->i_uid   = 0;
  root_inode->i_gid   = 0;
  root_inode->i_nlink = 1;  // real sys is 2
  root_inode->i_size  = 0;
  root_inode->i_mtime = (uint64_t)time(NULL);             
  
  root_inode->i_data      = NULL;
  root_inode->i_data_size = 0;
  root_inode->i_data_cap  = 0;

  /* root dentry */
  struct dentry *root_dentry = malloc(sizeof(struct dentry));
  if(!root_dentry)
  {
    free(root_inode);
    return -1;
  }

  memset(root_dentry, 0, sizeof(*root_dentry));
  root_dentry->d_name   = fs_strdup("/");
  root_dentry->d_parent = root_dentry; /* root parent to it self */
  root_dentry->d_inode  = root_inode;

  g_sb.s_root = root_dentry;
  g_cwd       = root_dentry;  
  return 0;
}

static int dentry_add_child(struct dentry *parent, struct dentry *child)
{
  if(parent == NULL || child == NULL)
  {
    return -1;
  }

  child->d_parent  = parent;
  child->d_sibling = parent->d_child;  /* into front */
  parent->d_child  = child;

  return 0;
}


static struct dentry *dentry_find_child(struct dentry *parent, const char *name)
{
  struct dentry *cur;

  if(parent == NULL || name == NULL)
  {
    return NULL;
  }

  for(cur = parent->d_child; cur != NULL; cur = cur->d_sibling)
  {
    if(cur->d_name != NULL && strcmp(cur->d_name, name) == 0)
    {
      return cur;
    }
  }

  return NULL;
}

int vfs_mkdir(const char *path)
{
  return vfs_mkdir_path_internal(path);
}

static int vfs_ls_dentry(struct dentry *dir)
{
  struct dentry *cur;

  if(dir == NULL)
  {
    return -1;
  }

  for(cur = dir->d_child; cur != NULL; cur = cur->d_sibling)
  {
    if(cur->d_name != NULL)
    {
      printf("%s\n", cur->d_name);
    }
  }

  return 0;
}

int vfs_ls(void)
{
  return vfs_ls_dentry(g_cwd);
}

int vfs_ls_path(const char *path)
{
  char buf[256];
  struct dentry *target;

  if(path == NULL || path[0] == '\0')
  {
    return -1;
  }

  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  trim(buf);
  if(buf[0] == '\0')
  {
    return -1;
  }

  target = vfs_lookup(buf);
  if(target == NULL || target->d_inode == NULL)
  {
    return -1;
  }

  if(target->d_inode->i_type != FS_INODE_DIR)
  {
    return -1; /* 不是目錄不能 ls */
  }

  return vfs_ls_dentry(target);
}

int vfs_cd(const char *path)
{
  char buf[256];
  struct dentry *target;

  if(path == NULL || path[0] == '\0')
  {
    return -1;
  }
  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  trim(buf);

  if(buf[0] == '\0')
  {
    return -1;
  }

  target = vfs_lookup(buf);
  if(target == NULL || target->d_inode == NULL)
  {
    return -1;
  }

  if(target->d_inode->i_type != FS_INODE_DIR)
  {
    return -1;  /* not dictionary */
  }

  g_cwd = target;
  return 0;
}

int vfs_get_cwd(char *buf, size_t size)
{
  const char *names[64];
  int depth = 0;
  struct dentry *d;
  size_t pos = 0;
  int i;

  if(buf == NULL || size == 0)
  {
    return -1;
  }

  buf[0] = '\0';

  if(g_cwd == NULL)
  {
    snprintf(buf, size, "?");
    return -1;
  }

  /* root 特例：直接回 "/" */
  if(g_cwd == g_sb.s_root)
  {
    snprintf(buf, size, "/");
    return 0;
  }

  /* 往上走，把每層名稱記到陣列（反向） */
  d = g_cwd;
  while(d != NULL && d != g_sb.s_root && depth < 64)
  {
    if(d->d_name != NULL)
    {
      names[depth++] = d->d_name;
    }
    else
    {
      names[depth++] = "?";
    }
    d = d->d_parent;
  }

  /* 開頭一定是一個 '/' */
  pos += snprintf(buf + pos, size - pos, "/");

  /* 從最上層一路往下組字串：/home/user/... */
  for(i = depth - 1; i >= 0; i--)
  {
    if(pos >= size)
    {
      buf[size - 1] = '\0';
      return -1;
    }

    if(i > 0)
    {
      pos += snprintf(buf + pos, size - pos, "%s/", names[i]);
    }
    else
    {
      pos += snprintf(buf + pos, size - pos, "%s", names[i]);
    }
  }

  return 0;
}


static void trim(char *s)
{
  char *p = s;
  int len;

  while(*p == ' ' || *p == '\t') p++;
  if(p != s) memmove(s, p, strlen(p) + 1);

  len = strlen(s);
  while(len > 0 && (s[len-1] == ' ' || s[len-1] == '\t'))
  {
    s[len-1] = '\0';
    len--;
  }
}

/* 把多個 '/' 合成一個 */
static void remove_multiple_slashes(char *s)
{
  char *dst = s;
  char *src = s;

  int slash = 0;

  while(*src)
  {
    if(*src == '/')
    {
      if(!slash)
      {
        *dst++ = '/';
      }
      slash = 1;
    }
    else
    {
      slash = 0;
      *dst++ = *src;
    }
    src++;
  }
  *dst = '\0';
}

/* 移除字串尾端的 '/' (保留 root "/" 特例) */
static void rstrip_slash(char *s)
{
  int len = strlen(s);
  while(len > 1 && s[len - 1] == '/')
  {
    s[len - 1] = '\0';
    len--;
  }
}

/* tokenize: 找下一段 token */
static char *next_token(char **p)
{
  char *s = *p;

  if(s == NULL || *s == '\0')
  {
    return NULL;
  }

  char *start = s;

  while(*s && *s != '/')
  {
    s++;
  }

  if(*s == '/')
  {
    *s = '\0';
    *p = s + 1;
  }
  else
  {
    *p = s;
  }

  return start;
}

struct dentry *vfs_lookup(const char *path)
{
  if(path == NULL || *path == '\0')
  {
    return NULL;
  }

  char buf[256];
  strncpy(buf, path, sizeof(buf)-1);
  buf[sizeof(buf)-1] = '\0';

  trim(buf);
  remove_multiple_slashes(buf);
  rstrip_slash(buf);

  struct dentry *cur;

  /* 絕對路徑 */
  if(buf[0] == '/')
  {
    cur = g_sb.s_root;
    if(buf[1] == '\0')
    {
      return cur;
    }
    char *p = buf + 1;

    char *tok;
    while((tok = next_token(&p)) != NULL)
    {
      if(strcmp(tok, ".") == 0)
      {
        continue;
      }
      else if(strcmp(tok, "..") == 0)
      {
        cur = cur->d_parent;
        continue;
      }
      else
      {
        cur = dentry_find_child(cur, tok);
        if(cur == NULL)
        {
          return NULL;
        }
      }
    }
    return cur;
  }
  else
  {
    /* 相對路徑 */
    cur = g_cwd;

    char *p = buf;
    char *tok;

    while((tok = next_token(&p)) != NULL)
    {
      if(strcmp(tok, ".") == 0)
      {
        continue;
      }
      else if(strcmp(tok, "..") == 0)
      {
        cur = cur->d_parent;
        continue;
      }
      else
      {
        cur = dentry_find_child(cur, tok);
        if(cur == NULL)
        {
          return NULL;
        }
      }
    }
    return cur;
  }
}

static int vfs_mkdir_path_internal(const char *path)
{
  struct inode  *inode;
  struct dentry *parent;
  struct dentry *dentry;

  char buf[256];
  char *last_slash;
  char *name;

  if(path == NULL || path[0] == '\0')
  {
    return -1;
  }

  /* 複製一份 path 來處理 */
  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  trim(buf);
  remove_multiple_slashes(buf);
  rstrip_slash(buf);

  if(buf[0] == '\0')
  {
    return -1;
  }

  /* 不允許對 "/" 做 mkdir */
  if(strcmp(buf, "/") == 0)
  {
    return -1;
  }

  /* 找最後一個 '/'，切成 parent path + 名稱 */
  last_slash = strrchr(buf, '/');

  if(last_slash == NULL)
  {
    /* 沒有 '/'，表示是相對路徑的單一名稱，parent = g_cwd */
    parent = g_cwd;
    name   = buf;
  }
  else
  {
    /* 有 '/' 的情況 */
    if(last_slash == buf)
    {
      /* 形如 "/name" */
      parent = g_sb.s_root;
      name   = last_slash + 1;
    }
    else
    {
      /* 形如 "/a/b/name" 或 "a/b/name" */
      *last_slash = '\0';          /* buf 現在變成 parent path */
      name        = last_slash + 1;

      parent = vfs_lookup(buf);
      if(parent == NULL)
      {
        return -1;
      }
    }
  }

  /* 名稱不能是空字串 */
  if(name[0] == '\0')
  {
    return -1;
  }

  /* parent 必須是目錄 */
  if(parent == NULL || parent->d_inode == NULL)
  {
    return -1;
  }
  if(parent->d_inode->i_type != FS_INODE_DIR)
  {
    return -1;
  }

  /* 檢查 parent 底下是否已有同名 */
  if(dentry_find_child(parent, name) != NULL)
  {
    return -1;
  }

  /* 建 inode */
  inode = malloc(sizeof(struct inode));
  if(inode == NULL)
  {
    return -1;
  }

  memset(inode, 0, sizeof(*inode));
  inode->i_ino  = 0;
  inode->i_type = FS_INODE_DIR;
  inode->i_mode = FS_IFDIR | 0755; /* 簡寫：0755 = rwxr-xr-x */
  
  inode->i_uid   = 0;
  inode->i_gid   = 0;
  inode->i_nlink = 1;
  inode->i_size  = 0;
  inode->i_mtime = (uint64_t)time(NULL);


  /* 建 dentry */
  dentry = malloc(sizeof(struct dentry));
  if(dentry == NULL)
  {
    free(inode);
    return -1;
  }

  memset(dentry, 0, sizeof(*dentry));
  dentry->d_name = fs_strdup(name);
  if(dentry->d_name == NULL)
  {
    free(dentry);
    free(inode);
    return -1;
  }

  dentry->d_inode = inode;

  if(dentry_add_child(parent, dentry) != 0)
  {
    free(dentry->d_name);
    free(dentry);
    free(inode);
    return -1;
  }

  return 0;
}

int vfs_create_file(const char *path)
{
  struct inode  *inode;
  struct dentry *parent;
  struct dentry *dentry;

  char buf[256];
  char *last_slash;
  char *name;

  if(path == NULL || path[0] == '\0')
  {
    return -1;
  }

  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  trim(buf);
  remove_multiple_slashes(buf);
  rstrip_slash(buf);

  if(buf[0] == '\0')
  {
    return -1;
  }
  if(strcmp(buf, "/") == 0)
  {
    return -1;
  }

  last_slash = strrchr(buf, '/');

  if(last_slash == NULL)
  {
    parent = g_cwd;
    name   = buf;
  }
  else
  {
    if(last_slash == buf)
    {
      parent = g_sb.s_root;
      name   = last_slash + 1;
    }
    else
    {
      *last_slash = '\0'; 
      name        = last_slash + 1;

      parent = vfs_lookup(buf);
      if(parent == NULL)
      {
        return -1;
      }
    }
  }

  if(name[0] == '\0')
  {
    return -1;
  }
  if(parent == NULL || parent->d_inode == NULL)
  {
    return -1;
  }
  if(parent->d_inode->i_type != FS_INODE_DIR)
  {
    return -1;
  }

  // check name exist
  if(dentry_find_child(parent, name) != NULL)
  {
    return -1;
  }

  // create inode(file)
  inode = malloc(sizeof(struct inode));
  if(inode == NULL)
  {
    return -1;
  }
  memset(inode, 0, sizeof(*inode));
  inode->i_ino       = 0;
  inode->i_type      = FS_INODE_FILE;
  inode->i_mode      = FS_IFREG | 0644;   /* -rw-r--r-- */

  inode->i_uid       = 0;
  inode->i_gid       = 0;
  inode->i_nlink     = 1;
  inode->i_size      = 0;
  inode->i_mtime     = (uint64_t)time(NULL);

  inode->i_data      = NULL;
  inode->i_data_size = 0;
  inode->i_data_cap  = 0;

  dentry = malloc(sizeof(struct dentry));
  if(dentry == NULL)
  {
    free(inode);
    return -1;
  }

  memset(dentry, 0, sizeof(*dentry));
  dentry->d_name = fs_strdup(name);
  if(dentry->d_name == NULL)
  {
    free(dentry);
    free(inode);
    return -1;
  }

  dentry->d_inode = inode;

  if(dentry_add_child(parent, dentry) != 0)
  {
    free(dentry->d_name);
    free(dentry);
    free(inode);
    return -1;
  }

  return 0;
}

int vfs_write_all(const char *path, const char *data)
{
  struct dentry *dent;
  struct inode  *inode;
  size_t len;
  char *buf;

  if(path == NULL || data == NULL)
  {
    return -1;
  }

  dent = vfs_lookup(path);
  if(dent == NULL || dent->d_inode == NULL)
  {
    return -1;
  }

  inode = dent->d_inode;

  if(inode->i_type != FS_INODE_FILE)
  {
    return -1;  /* 不是檔案，不能寫 */
  }

  len = strlen(data);

  buf = malloc(len + 1);
  if(buf == NULL)
  {
    return -1;
  }

  memcpy(buf, data, len + 1);  /* 包含 '\0' */

  /* 釋放舊內容 */
  if(inode->i_data != NULL)
  {
    free(inode->i_data);
  }

  inode->i_data      = buf;
  inode->i_data_size = len;
  inode->i_data_cap  = len + 1;
  inode->i_size      = (fs_off_t)len;
  inode->i_mtime     = (uint64_t)time(NULL);

  return 0;
}


int vfs_cat(const char *path)
{
  struct dentry *dent;
  struct inode  *inode;

  if(path == NULL)
  {
    return -1;
  }

  dent = vfs_lookup(path);
  if(dent == NULL || dent->d_inode == NULL)
  {
    return -1;
  }

  inode = dent->d_inode;

  if(inode->i_type != FS_INODE_FILE)
  {
    return -1;  /* 不是檔案 */
  }

  if(inode->i_data != NULL)
  {
    printf("%s\n", inode->i_data);
  }

  return 0;
}

static int dentry_remove_child(struct dentry *parent, struct dentry *child)
{
  struct dentry *prev = NULL;
  struct dentry *cur;

  if(parent == NULL || child == NULL)
  {
    return -1;
  }

  cur = parent->d_child;
  while(cur != NULL && cur != child)
  {
    prev = cur;
    cur  = cur->d_sibling;
  }

  if(cur == NULL)
  {
    /* 沒找到 child */
    return -1;
  }

  if(prev == NULL)
  {
    /* child 是第一個 */
    parent->d_child = cur->d_sibling;
  }
  else
  {
    prev->d_sibling = cur->d_sibling;
  }

  child->d_parent  = NULL;
  child->d_sibling = NULL;
  return 0;
}

int vfs_rm(const char *path)
{
  char buf[256];
  struct dentry *dent;
  struct dentry *parent;
  struct inode  *inode;

  if(path == NULL || path[0] == '\0')
  {
    return -1;
  }

  /* 正規化路徑 */
  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  trim(buf);
  remove_multiple_slashes(buf);
  rstrip_slash(buf);

  if(buf[0] == '\0')
  {
    return -1;
  }

  /* 不能 rm "/" */
  if(strcmp(buf, "/") == 0)
  {
    return -1;
  }

  dent = vfs_lookup(buf);
  if(dent == NULL || dent->d_inode == NULL)
  {
    return -1;
  }

  inode  = dent->d_inode;
  parent = dent->d_parent;

  /* 不能刪 root（root 的 parent 指向自己） */
  if(dent == g_sb.s_root || parent == NULL || parent == dent)
  {
    return -1;
  }

  /* 只允許刪檔案，目錄請用 rmdir */
  if(inode->i_type != FS_INODE_FILE)
  {
    return -1;
  }

  /* 從 parent 的 child list 移除 */
  if(dentry_remove_child(parent, dent) != 0)
  {
    return -1;
  }

  /* 釋放檔案內容與 inode/dentry */
  if(inode->i_data != NULL)
  {
    free(inode->i_data);
  }
  free(inode);

  if(dent->d_name != NULL)
  {
    free(dent->d_name);
  }
  free(dent);

  return 0;
}

int vfs_rmdir(const char *path)
{
  char buf[256];
  struct dentry *dent;
  struct dentry *parent;
  struct inode  *inode;

  if(path == NULL || path[0] == '\0')
  {
    return -1;
  }

  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  trim(buf);
  remove_multiple_slashes(buf);
  rstrip_slash(buf);

  if(buf[0] == '\0')
  {
    return -1;
  }

  /* 不能 rmdir "/" */
  if(strcmp(buf, "/") == 0)
  {
    return -1;
  }

  dent = vfs_lookup(buf);
  if(dent == NULL || dent->d_inode == NULL)
  {
    return -1;
  }

  inode  = dent->d_inode;
  parent = dent->d_parent;

  /* 不能刪 root */
  if(dent == g_sb.s_root || parent == NULL || parent == dent)
  {
    return -1;
  }

  /* 必須是目錄 */
  if(inode->i_type != FS_INODE_DIR)
  {
    return -1;
  }

  /* 目錄必須是空的，不能有 child */
  if(dent->d_child != NULL)
  {
    return -1;
  }

  /* 從 parent 的 child list 移除 */
  if(dentry_remove_child(parent, dent) != 0)
  {
    return -1;
  }

  /* 釋放 inode / dentry */
  free(inode);

  if(dent->d_name != NULL)
  {
    free(dent->d_name);
  }
  free(dent);

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

  if(dir == NULL)
  {
    return -1;
  }

  for(cur = dir->d_child; cur != NULL; cur = cur->d_sibling)
  {
    struct inode *inode = cur->d_inode;
    char mode_str[11] = "----------";
    char time_str[32] = "";
    time_t t;
    struct tm *tm;

    if(inode == NULL)
    {
      continue;
    }

    fs_mode_to_str(inode->i_mode, mode_str);

    t  = (time_t)inode->i_mtime;
    tm = localtime(&t);
    if(tm)
    {
      /* e.g. "Apr  7 09:45" */
      strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm);
    }

    printf("%s %2u %4u %4u %8lld %s %s\n",
           mode_str,
           (unsigned)inode->i_nlink,
           (unsigned)inode->i_uid,
           (unsigned)inode->i_gid,
           (long long)inode->i_size,
           time_str,
           cur->d_name ? cur->d_name : "?");
  }

  return 0;
}

int vfs_ls_long(void)
{
  return vfs_ls_long_dentry(g_cwd);
}

int vfs_ls_long_path(const char *path)
{
  char buf[256];
  struct dentry *target;

  if(path == NULL || path[0] == '\0')
  {
    return -1;
  }

  strncpy(buf, path, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  trim(buf);
  if(buf[0] == '\0')
  {
    return -1;
  }

  target = vfs_lookup(buf);
  if(target == NULL || target->d_inode == NULL)
  {
    return -1;
  }

  if(target->d_inode->i_type != FS_INODE_DIR)
  {
    return -1;
  }

  return vfs_ls_long_dentry(target);
}


/* Function define done */
