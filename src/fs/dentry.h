#ifndef _DENRTY_H_
#define _DENRTY_H_

struct inode;

struct dentry 
{
  char *d_name;
  struct dentry *d_parent;
  struct inode  *d_inode;

  struct dentry *d_child;   // next child
  struct dentry *d_sibling; // next brother
};

#endif /* _DENRTY_H_ */