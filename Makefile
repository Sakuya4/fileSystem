CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -g

INCLUDES := -Iinc -Iinc/fs

TARGET  := VFS

SRC_DIR := src
FS_DIR  := $(SRC_DIR)/fs

SRCS := \
    $(SRC_DIR)/main.c \
    $(SRC_DIR)/shell.c \
    $(FS_DIR)/vfs_cores.c \
    $(FS_DIR)/vfs.c \
    $(FS_DIR)/path.c \
    $(FS_DIR)/vfs_dir.c \
    $(FS_DIR)/vfs_file.c \
    $(FS_DIR)/block.c \
    $(FS_DIR)/meta.c \
    $(FS_DIR)/perm.c \
    $(FS_DIR)/vfs_vim.c \
    $(FS_DIR)/vfs_io.c

OBJS := $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

ifeq ($(OS),Windows_NT)
clean:
	-del /Q $(subst /,\,$(OBJS)) $(TARGET) 2>nul
else
clean:
	rm -f $(OBJS) $(TARGET)
endif

run: all
	./$(TARGET)
