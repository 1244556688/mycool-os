CC = gcc
AS = nasm
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-builtin -fno-pie -fno-stack-protector -nostdlib
LDFLAGS = -m32 -nostdlib -no-pie -T linker.ld
ASFLAGS = -f elf32

# 目錄與檔案
SRC_DIR = src
OBJ_DIR = obj
ISO_DIR = isodir
KERNEL_BIN = $(ISO_DIR)/boot/kernel.bin
ISO_NAME = bootable.iso

# 原始碼與目標檔
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
S_SOURCES = $(wildcard $(SRC_DIR)/*.asm)
OBJS = $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%_asm.o, $(S_SOURCES)) \
       $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_SOURCES))

all: $(ISO_NAME)

$(OBJ_DIR)/%_asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(OBJ_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(OBJS)
	@mkdir -p $(dir $(KERNEL_BIN))
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(ISO_NAME): $(KERNEL_BIN)
	grub-mkrescue -o $(ISO_NAME) $(ISO_DIR)

clean:
	rm -rf $(OBJ_DIR) $(KERNEL_BIN) $(ISO_NAME)

.PHONY: all clean
