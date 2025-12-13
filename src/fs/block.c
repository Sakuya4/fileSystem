/*standard lib */
#include <string.h>
#include "block.h"
/*standard lib done */

/* RAM BLOCK Device (temp) */
static uint8_t block_data[BLOCK_COUNT][BLOCK_SIZE];
static uint8_t block_bitmap[BLOCK_COUNT]; /* 0 = free, 1 = used */
/* RAM BLOCK Device (temp) */

/* define function */
int block_init(void)
{
  memset(block_data, 0, sizeof(block_data));
  memset(block_bitmap, 0, sizeof(block_bitmap));
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
  if (!buf)
    return -1;
  if (blkno < 0 || blkno >= BLOCK_COUNT)
    return -1;
  if (block_bitmap[blkno] == 0)
    return -1; /* reading free block = error */

  memcpy(buf, block_data[blkno], BLOCK_SIZE);
  return 0;
}

int block_write(int blkno, const void *buf)
{
  if (!buf)
    return -1;
  if (blkno < 0 || blkno >= BLOCK_COUNT)
    return -1;
  if (block_bitmap[blkno] == 0)
    return -1; /* writing free block = error */

  memcpy(block_data[blkno], buf, BLOCK_SIZE);
  return 0;
}

/* define function */
