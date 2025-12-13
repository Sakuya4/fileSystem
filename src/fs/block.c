/*standard lib */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
/*standard lib done*/

#include "block.h"


/* RAM BLOCK Device (temp) */
static uint8_t block_data[BLOCK_COUNT][BLOCK_SIZE];
static uint8_t block_bitmap[BLOCK_COUNT]; /* 0 = free, 1 = used */
/* RAM BLOCK Device (temp) */

/* define function */
int block_init(void)
{
  // memset(block_data, 0, sizeof(block_data));
  // memset(block_bitmap, 0, sizeof(block_bitmap));
  return 0;
}

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "block.h"

/* 你原本就有的全域 */
static uint8_t block_data[BLOCK_COUNT][BLOCK_SIZE];
static uint8_t block_bitmap[BLOCK_COUNT]; /* 0 free, 1 used */

/* 可選：給 image 一個 magic，避免讀錯檔 */
#define IMG_MAGIC 0x56465331u /* 'VFS1' */

typedef struct {
    uint32_t magic;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t reserved;
} img_hdr_t;

int block_reserve(int blkno)
{
    if (blkno < 0 || blkno >= (int)BLOCK_COUNT) return -1;
    block_bitmap[blkno] = 1;
    return 0;
}

int block_save_image(const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) return -1;

    img_hdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = IMG_MAGIC;
    hdr.block_size = BLOCK_SIZE;
    hdr.block_count = BLOCK_COUNT;

    /* header */
    if (fwrite(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr)) { fclose(fp); return -1; }

    /* bitmap */
    if (fwrite(block_bitmap, 1, BLOCK_COUNT, fp) != BLOCK_COUNT) { fclose(fp); return -1; }

    /* data blocks */
    size_t total = (size_t)BLOCK_COUNT * (size_t)BLOCK_SIZE;
    if (fwrite(block_data, 1, total, fp) != total) { fclose(fp); return -1; }

    fclose(fp);
    return 0;
}

int block_load_image(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        /* 第一次沒有檔案很正常 */
        return -1;
    }

    img_hdr_t hdr;
    if (fread(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr)) { fclose(fp); return -1; }

    if (hdr.magic != IMG_MAGIC ||
        hdr.block_size != BLOCK_SIZE ||
        hdr.block_count != BLOCK_COUNT) {
        fclose(fp);
        return -1;
    }

    if (fread(block_bitmap, 1, BLOCK_COUNT, fp) != BLOCK_COUNT) { fclose(fp); return -1; }

    size_t total = (size_t)BLOCK_COUNT * (size_t)BLOCK_SIZE;
    if (fread(block_data, 1, total, fp) != total) { fclose(fp); return -1; }

    fclose(fp);
    return 0;
}


size_t block_total_blocks(void)
{
  return (size_t)BLOCK_COUNT;
}

size_t block_used_blocks(void)
{
  size_t used = 0;
  for(size_t i = 0; i < BLOCK_COUNT; i++)
  {
    if (block_bitmap[i])
    used++;
 }
  return used;
}

size_t block_free_blocks(void)
{
  return block_total_blocks() - block_used_blocks();
}

size_t block_total_size(void)
{
  return (size_t)BLOCK_COUNT * (size_t)BLOCK_SIZE;
}

size_t block_used_size(void)
{
  return block_used_blocks() * (size_t)BLOCK_SIZE;
}

size_t block_free_size(void)
{
  return block_free_blocks() * (size_t)BLOCK_SIZE;
}

int block_alloc(void)
{
    for (int i = 0; i < BLOCK_COUNT; i++)
    {
        if (block_bitmap[i] == 0)
        {
            block_bitmap[i] = 1;
            memset(block_data[i], 0, BLOCK_SIZE);
            return i;
        }
    }
    return -1; /* full */
}

void block_free(int blkno)
{
    if (blkno < 0 || blkno >= BLOCK_COUNT)
        return;

    block_bitmap[blkno] = 0;
    /* 可選：free 時清掉資料 */
    memset(block_data[blkno], 0, BLOCK_SIZE);
}

int block_read(int blkno, void *buf)
{
    if (!buf) return -1;
    if (blkno < 0 || blkno >= (int)BLOCK_COUNT) return -1;
    memcpy(buf, block_data[blkno], BLOCK_SIZE);
    return 0;
}

int block_write(int blkno, const void *buf)
{
    if (!buf) return -1;
    if (blkno < 0 || blkno >= (int)BLOCK_COUNT) return -1;
    memcpy(block_data[blkno], buf, BLOCK_SIZE);
    return 0;
}

/* define function */
