#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "meta.h"
#include "block.h"
#include "vfs_internal.h"
#include "dentry.h"
#include "inode.h"

/* ---------- on-disk layout ---------- */
#define META_MAGIC 0x4D455441u /* 'META' */
#define META_VER   1

#define META_BLK_HEADER 0
#define META_BLK_ENTRIES_START 1
#define META_MAX_ENTRIES 1024

/* root entries only (for Phase 4-2) */
typedef struct {
    uint32_t magic;
    uint32_t ver;
    uint32_t entry_count;
    uint32_t reserved;
} meta_header_t;

#define NAME_MAX_ONDISK 60

typedef struct {
    uint8_t  used;          /* 0 free, 1 used */
    uint8_t  type;          /* FS_INODE_FILE / FS_INODE_DIR */
    uint16_t reserved0;
    uint32_t size;          /* file size */
    int32_t  blocks[DIRECT_BLOCKS]; /* direct blocks */
    int32_t  parent;
    char     name[NAME_MAX_ONDISK]; /* null-terminated if fits */
} meta_entry_t;

static void entry_clear(meta_entry_t *e) {
    memset(e, 0, sizeof(*e));
    for (int i = 0; i < DIRECT_BLOCKS; i++) e->blocks[i] = -1;
}

static void inode_init_blocks(struct inode *ino) {
    for (int i = 0; i < DIRECT_BLOCKS; i++) ino->i_block[i] = -1;
}

static meta_entry_t g_entries[META_MAX_ENTRIES];
static uint32_t g_entry_count;


/* count root children */
static uint32_t count_root_children(void) {
    struct super_block *sb = fs_get_super();
    if (!sb || !sb->s_root) return 0;

    uint32_t n = 0;
    for (struct dentry *cur = sb->s_root->d_child; cur; cur = cur->d_sibling) {
        if (cur->d_name && cur->d_inode) n++;
    }
    return n;
}

//DFS
static void save_dentry_recursive(struct dentry *d, int parent_idx)
{
    if (!d || !d->d_inode || !d->d_name) return;
    if (g_entry_count >= META_MAX_ENTRIES) return;

    int my_idx = g_entry_count++;
    meta_entry_t *e = &g_entries[my_idx];
    entry_clear(e);

    e->used   = 1;
    e->type   = (uint8_t)d->d_inode->i_type;
    e->size   = (uint32_t)d->d_inode->i_size;
    e->parent = parent_idx;

    for (int i = 0; i < DIRECT_BLOCKS; i++) {
        e->blocks[i] = d->d_inode->i_block[i];
    }

    strncpy(e->name, d->d_name, NAME_MAX_ONDISK - 1);
    e->name[NAME_MAX_ONDISK - 1] = '\0';

    /* recurse into children */
    for (struct dentry *c = d->d_child; c; c = c->d_sibling) {
        save_dentry_recursive(c, my_idx);
    }
}

int meta_save(void)
{
    uint8_t buf[BLOCK_SIZE];
    meta_header_t hdr;

    struct super_block *sb = fs_get_super();
    if (!sb || !sb->s_root) return -1;

    /* 1) DFS collect */
    g_entry_count = 0;
    for (struct dentry *c = sb->s_root->d_child; c; c = c->d_sibling) {
        save_dentry_recursive(c, -1);
    }

    /* 2) reserve meta blocks */
    block_reserve(META_BLK_HEADER);

    uint32_t entry_bytes  = g_entry_count * sizeof(meta_entry_t);
    uint32_t entry_blocks = (entry_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for (uint32_t b = 0; b < entry_blocks; b++) {
        block_reserve(META_BLK_ENTRIES_START + b);
    }

    /* 3) write header */
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = META_MAGIC;
    hdr.ver   = META_VER;
    hdr.entry_count = g_entry_count;

    memset(buf, 0, sizeof(buf));
    memcpy(buf, &hdr, sizeof(hdr));
    if (block_write(META_BLK_HEADER, buf) != 0) return -1;

    /* 4) write entries */
    uint32_t blkno = META_BLK_ENTRIES_START;
    uint32_t offset = 0;
    memset(buf, 0, sizeof(buf));

    for (uint32_t i = 0; i < g_entry_count; i++) {
        if (offset + sizeof(meta_entry_t) > BLOCK_SIZE) {
            if (block_write(blkno, buf) != 0) return -1;
            blkno++;
            offset = 0;
            memset(buf, 0, sizeof(buf));
        }

        memcpy(buf + offset, &g_entries[i], sizeof(meta_entry_t));
        offset += sizeof(meta_entry_t);
    }

    if (g_entry_count > 0) {
        if (block_write(blkno, buf) != 0) return -1;
    }

    return 0;
}



int meta_load(void)
{
    struct dentry *dent_list[META_MAX_ENTRIES] = {0};
    meta_entry_t   entry_list[META_MAX_ENTRIES];

    uint8_t buf[BLOCK_SIZE];
    meta_header_t hdr;

    struct super_block *sb = fs_get_super();
    if (!sb || !sb->s_root) return -1;

    if (block_read(META_BLK_HEADER, buf) != 0) return -1;
    memcpy(&hdr, buf, sizeof(hdr));

    if (hdr.magic != META_MAGIC || hdr.ver != META_VER) {
        return 0; // empty fs
    }

    // reserve meta blocks
    block_reserve(META_BLK_HEADER);
    uint32_t entry_bytes  = hdr.entry_count * (uint32_t)sizeof(meta_entry_t);
    uint32_t entry_blocks = (entry_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (uint32_t b = 0; b < entry_blocks; b++) {
        block_reserve((int)(META_BLK_ENTRIES_START + b));
    }

    uint32_t to_load = hdr.entry_count;
    if (to_load == 0) return 0;

    // 讀 entries：先填 entry_list[]
    uint32_t blkno = META_BLK_ENTRIES_START;
    uint32_t offset = 0;
    if (block_read((int)blkno, buf) != 0) return -1;

    for (uint32_t i = 0; i < to_load; i++) {
        if (offset + sizeof(meta_entry_t) > BLOCK_SIZE) {
            blkno++;
            offset = 0;
            if (block_read((int)blkno, buf) != 0) return -1;
        }
        memcpy(&entry_list[i], buf + offset, sizeof(meta_entry_t));
        offset += (uint32_t)sizeof(meta_entry_t);
    }

    // 第 1 pass：create dent/inode，但先不掛 tree
    for (uint32_t i = 0; i < to_load; i++) {
        meta_entry_t *e = &entry_list[i];
        if (!e->used) continue;

        struct inode *ino = (struct inode*)calloc(1, sizeof(struct inode));
        if (!ino) return -1;

        ino->i_type  = (fs_inode_type_t)e->type;
        ino->i_size  = (size_t)e->size;
        ino->i_mtime = (uint64_t)time(NULL);
        ino->i_mode  = (ino->i_type == FS_INODE_DIR) ? (FS_IFDIR | 0755) : (FS_IFREG | 0644);

        inode_init_blocks(ino);
        for (int k = 0; k < DIRECT_BLOCKS; k++) {
            ino->i_block[k] = e->blocks[k];
            if (e->blocks[k] >= 0) block_reserve(e->blocks[k]); // 保險：把檔案 data block 標成 used
        }

        struct dentry *dent = (struct dentry*)calloc(1, sizeof(struct dentry));
        if (!dent) { free(ino); return -1; }

        dent->d_name = fs_strdup(e->name);
        if (!dent->d_name) { free(dent); free(ino); return -1; }

        dent->d_inode = ino;
        dent_list[i] = dent;
    }

    // 第 2 pass：依 parent 掛起來
    for (uint32_t i = 0; i < to_load; i++) {
        meta_entry_t *e = &entry_list[i];
        if (!e->used) continue;
        if (!dent_list[i]) continue;

        if (e->parent < 0) {
            dentry_add_child(sb->s_root, dent_list[i]);
        } else {
            int p = e->parent;
            if (p >= 0 && p < (int)to_load && dent_list[p]) {
                dentry_add_child(dent_list[p], dent_list[i]);
            } else {
                // parent 壞掉：保底掛回 root
                dentry_add_child(sb->s_root, dent_list[i]);
            }
        }
    }

    return 0;
}
