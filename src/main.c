#include <stdio.h>

#include "vfs.h"
#include "meta.h"
#include "shell.h"
#include "block.h"


int main(void)
{
    block_init();

    if (block_load_image("disk.img") != 0) {
        printf("first run, empty disk\n");
    }

    fs_init();
    meta_load();

    run_shell();

    meta_save();
    block_save_image("disk.img");
}
