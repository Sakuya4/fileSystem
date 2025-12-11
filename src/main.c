#include <stdio.h>

#include "fs/vfs.h"
#include "shell.h"

int main(void)
{
  if (fs_init() != 0)
  {
    printf("fs_init failed\n");
    return 1;
  }

  printf("Filesystem mounted. Type commands.\n");
  printf("Available: mkdir, cd, ls [path], touch, write, cat, rm, rmdir, exit\n");
  printf("touch <path>, write <path> <text>, cat <path>, rm <path>, rmdir <path>, exit\n");
  printf("----------------------------------------------\n");

  run_shell();
  return 0;
}