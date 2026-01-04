#ifndef _SUPER_H_
#define _SUPER_H_

#include <stdint.h>

struct dentry;

struct super_block 
{
  uint32_t s_magic;
  struct dentry *s_root;    
};

#endif /* _SUPER_H_ */
