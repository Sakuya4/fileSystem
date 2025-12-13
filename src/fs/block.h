#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <stddef.h>
#include <stdint.h>

#define BLOCK_SIZE     512          /* bytes per block */
#define BLOCK_COUNT    1024         /* total blocks (512 KB total) */


// Init / info
int  block_init(void);
size_t block_total_size(void);      /* total bytes */
size_t block_used_size(void);       /* used bytes */
size_t block_free_size(void);       /* free bytes */
size_t block_total_blocks(void);
size_t block_used_blocks(void);
size_t block_free_blocks(void);

// Allocate / free
int  block_alloc(void);              /* return block index, -1 if full */
void block_free(int blkno);

// IO
int  block_read(int blkno, void *buf);
int  block_write(int blkno, const void *buf);

#endif /* _BLOCK_H_ */