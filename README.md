# fileSystem

### Core Objective

- Supported Commands: mkdir, cd, ls, touch, cat, rm, rmdir, df.



### Architecture

final/
  Makefile
  README.md
  Note.md
  disk.img              
  VFS.exe               
  src/
    main.c
    shell.c, shell.h
    UI.c, UI.h
    fs/
      vfs.c
      vfs_cores.c
      vfs_dir.c
      vfs_file.c
      vfs_vim.c
      path.c
      perm.c
      meta.c
      block.c
      *.h