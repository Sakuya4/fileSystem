CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -g

INCLUDES := -Isrc -Isrc/fs

TARGET  := VFS

SRC_DIR := src
FS_DIR  := $(SRC_DIR)/fs

SRCS := \
    $(SRC_DIR)/main.c \
    $(SRC_DIR)/shell.c \
    $(FS_DIR)/vfs_cores.c \
    $(FS_DIR)/vfs.c \
    $(FS_DIR)/path.c \
    $(FS_DIR)/vfs_dir.c

OBJS := $(SRCS:.c=.o)

.PHONY: all clean run


all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^


%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


# clean:
# 	rm -f $(OBJS) $(TARGET)

clean:
	-del /Q $(OBJS) $(TARGET) 2>nul

run: all
	./$(TARGET)
