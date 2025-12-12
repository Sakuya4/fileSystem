/*
 * vfs_dir.c — Directory-related VFS operations
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "vfs.h"
#include "vfs_internal.h"
#include "inode.h"
#include "dentry.h"
#include "path.h"

/* ---------------------------------------------------------
 *  Internal helpers for mkdir, ls, ls -l
 * --------------------------------------------------------- */
static int vfs_mkdir_path_internal(const char *path);
static int vfs_ls_dentry(struct dentry *dir);
static void fs_mode_to_str(fs_mode_t mode, char out[11]);
static int vfs_ls_long_dentry(struct dentry *dir);

/* external global (defined in vfs_core.c) */
extern struct dentry *g_cwd;
extern struct super_block g_sb;

/* =========================================================
 *  mkdir <path>
 * ========================================================= */
int vfs_mkdir(const char *path)
{
    return vfs_mkdir_path_internal(path);
}

static int vfs_mkdir_path_internal(const char *path)
{
    struct inode  *inode;
    struct dentry *parent;
    struct dentry *dentry;

    char buf[256];
    char *last_slash;
    char *name;

    if (path == NULL || path[0] == '\0')
        return -1;

    /* Standard path normalization */
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trim(buf);
    remove_multiple_slashes(buf);
    rstrip_slash(buf);

    if (buf[0] == '\0')
        return -1;

    if (strcmp(buf, "/") == 0)
        return -1;

    /* Find parent path + name */
    last_slash = strrchr(buf, '/');
    if (last_slash == NULL) {
        parent = g_cwd;
        name   = buf;
    } else {
        if (last_slash == buf) {
            parent = g_sb.s_root;
            name   = last_slash + 1;
        } else {
            *last_slash = '\0';
            name = last_slash + 1;
            parent = vfs_lookup(buf);
            if (parent == NULL)
                return -1;
        }
    }

    if (name[0] == '\0')
        return -1;

    if (!parent || !parent->d_inode)
        return -1;

    if (parent->d_inode->i_type != FS_INODE_DIR)
        return -1;

    if (dentry_find_child(parent, name))
        return -1;

    /* create inode */
    inode = malloc(sizeof(struct inode));
    if (!inode)
        return -1;

    memset(inode, 0, sizeof(*inode));
    inode->i_type = FS_INODE_DIR;
    inode->i_mode = FS_IFDIR | 0755;
    inode->i_uid  = 0;
    inode->i_gid  = 0;
    inode->i_nlink = 1;
    inode->i_mtime = (uint64_t)time(NULL);

    /* create dentry */
    dentry = malloc(sizeof(struct dentry));
    if (!dentry) {
        free(inode);
        return -1;
    }

    memset(dentry, 0, sizeof(*dentry));
    dentry->d_name = fs_strdup(name);
    if (!dentry->d_name) {
        free(dentry);
        free(inode);
        return -1;
    }

    dentry->d_inode = inode;

    if (dentry_add_child(parent, dentry) != 0) {
        free(dentry->d_name);
        free(dentry);
        free(inode);
        return -1;
    }

    return 0;
}

/* =========================================================
 *  cd <path>
 * ========================================================= */
int vfs_cd(const char *path)
{
    char buf[256];
    struct dentry *target;

    if (!path || path[0] == '\0')
        return -1;

    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    trim(buf);
    if (buf[0] == '\0')
        return -1;

    target = vfs_lookup(buf);
    if (!target || !target->d_inode)
        return -1;

    if (target->d_inode->i_type != FS_INODE_DIR)
        return -1;

    g_cwd = target;
    return 0;
}

/* =========================================================
 *  ls (short)
 * ========================================================= */
static int vfs_ls_dentry(struct dentry *dir)
{
    struct dentry *cur;

    if (!dir)
        return -1;

    for (cur = dir->d_child; cur != NULL; cur = cur->d_sibling) {
        if (cur->d_name)
            printf("%s\n", cur->d_name);
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

    if (!path || path[0] == '\0')
        return -1;

    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trim(buf);

    if (buf[0] == '\0')
        return -1;

    target = vfs_lookup(buf);
    if (!target || !target->d_inode)
        return -1;

    if (target->d_inode->i_type != FS_INODE_DIR)
        return -1;

    return vfs_ls_dentry(target);
}

/* =========================================================
 *  ls -l
 * ========================================================= */
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

    if (!dir)
        return -1;

    for (cur = dir->d_child; cur != NULL; cur = cur->d_sibling) {

        struct inode *inode = cur->d_inode;
        char mode_str[11];
        char time_str[32] = "";
        time_t t;
        struct tm *tm;

        if (!inode)
            continue;

        fs_mode_to_str(inode->i_mode, mode_str);

        t = (time_t)inode->i_mtime;
        tm = localtime(&t);
        if (tm) {
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

    if (!path || path[0] == '\0')
        return -1;

    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trim(buf);

    if (buf[0] == '\0')
        return -1;

    target = vfs_lookup(buf);
    if (!target || !target->d_inode)
        return -1;

    if (target->d_inode->i_type != FS_INODE_DIR)
        return -1;

    return vfs_ls_long_dentry(target);
}
