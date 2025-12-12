# Minix RV64 Makefile
# Target: MilkV Duo CV1800B

CROSS_COMPILE = riscv64-unknown-elf-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

ARCH = riscv64
# Board selection: milkv-duo or qemu-virt
BOARD ?= qemu-virt

# Compiler flags
CFLAGS = -march=rv64gc -mabi=lp64d -mcmodel=medany
CFLAGS += -nostdlib -fno-builtin -fno-strict-aliasing
CFLAGS += -Wall -Wextra -Werror -O2 -g
CFLAGS += -Iinclude -Iarch/$(ARCH)/include -Iinclude/asm

# Board-specific flags
ifeq ($(BOARD), qemu-virt)
    CFLAGS += -DBOARD=2
    LDFLAGS = -T arch/$(ARCH)/kernel_qemu.ld
else ifeq ($(BOARD), milkv-duo)
    CFLAGS += -DBOARD=1
    LDFLAGS = -T arch/$(ARCH)/kernel.ld
endif

# Kernel image
KERNEL_IMAGE = minix-rv64.bin
KERNEL_ELF = minix-rv64.elf

# Directories
ARCH_DIR = arch/$(ARCH)
DRIVER_DIR = drivers
FS_DIR = fs
KERNEL_DIR = kernel
MM_DIR = mm
LIB_DIR = lib

# Source files
ASM_SRCS = $(ARCH_DIR)/boot/start.S $(ARCH_DIR)/kernel/trap_asm.S $(ARCH_DIR)/kernel/swtch.S
C_SRCS = $(ARCH_DIR)/kernel/main.c \
         $(ARCH_DIR)/kernel/trap.c \
         $(ARCH_DIR)/mm/mmu.c \
         $(ARCH_DIR)/mm/page_alloc.c \
         $(ARCH_DIR)/mm/pgtable.c \
         $(ARCH_DIR)/mm/slab.c \
         $(ARCH_DIR)/mm/vmalloc.c \
         $(KERNEL_DIR)/sched_new.c \
         $(KERNEL_DIR)/fork.c \
         $(KERNEL_DIR)/exit.c \
         $(KERNEL_DIR)/exec.c \
         $(KERNEL_DIR)/init_proc.c \
         $(KERNEL_DIR)/syscalls.c \
         $(KERNEL_DIR)/board.c \
         $(KERNEL_DIR)/drivers.c \
         $(KERNEL_DIR)/shell.c \
         $(LIB_DIR)/printk.c \
         $(LIB_DIR)/string.c \
         $(DRIVER_DIR)/char/uart.c \
         $(DRIVER_DIR)/block/blockdev.c \
         $(FS_DIR)/vfs.c \
         $(FS_DIR)/fat.c \
         $(FS_DIR)/fat32.c \
         $(FS_DIR)/ext2.c \
         $(FS_DIR)/ext3.c \
         $(FS_DIR)/ext4.c \
         $(FS_DIR)/devfs.c \
         $(FS_DIR)/ramfs.c

OBJS = $(ASM_SRCS:.S=.o) $(C_SRCS:.c=.o)

# QEMU options
QEMU = qemu-system-riscv64
QEMU_MACHINE = virt
QEMU_CPU = rv64
QEMU_SMP = 1
QEMU_MEMORY = 128M
QEMU_BIOS = none
QEMU_SERIAL = stdio
# Network disabled until network stack is implemented
# QEMU_EXTRA_ARGS = -device virtio-net-device,netdev=net0 -netdev user,id=net0,hostfwd=tcp::2222-:22
QEMU_EXTRA_ARGS =

.PHONY: all clean qemu qemu-debug qemu-gdb

all: $(KERNEL_IMAGE)

$(KERNEL_IMAGE): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.S
	$(CC) $(CFLAGS) -D__ASSEMBLY__ -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(KERNEL_ELF) $(KERNEL_IMAGE)

qemu: $(KERNEL_IMAGE)
	$(QEMU) \
		-machine $(QEMU_MACHINE) \
		-cpu $(QEMU_CPU) \
		-smp $(QEMU_SMP) \
		-m $(QEMU_MEMORY) \
		-bios $(QEMU_BIOS) \
		-kernel $(KERNEL_ELF) \
		-serial $(QEMU_SERIAL) \
		-nographic \
		-monitor none \
		$(QEMU_EXTRA_ARGS)

qemu-debug: $(KERNEL_IMAGE)
	$(QEMU) \
		-machine $(QEMU_MACHINE) \
		-cpu $(QEMU_CPU) \
		-smp $(QEMU_SMP) \
		-m $(QEMU_MEMORY) \
		-bios $(QEMU_BIOS) \
		-kernel $(KERNEL_ELF) \
		-serial $(QEMU_SERIAL) \
		-nographic \
		-monitor none \
		-s -S \
		$(QEMU_EXTRA_ARGS)

qemu-gdb: $(KERNEL_IMAGE)
	$(QEMU) \
		-machine $(QEMU_MACHINE) \
		-cpu $(QEMU_CPU) \
		-smp $(QEMU_SMP) \
		-m $(QEMU_MEMORY) \
		-bios $(QEMU_BIOS) \
		-kernel $(KERNEL_ELF) \
		-serial mon:stdio \
		-s -S \
		$(QEMU_EXTRA_ARGS)

gdb:
	$(CROSS_COMPILE)gdb $(KERNEL_ELF) \
		-ex "target remote localhost:1234" \
		-ex "set architecture riscv:rv64" \
		-ex "layout split"

help:
	@echo "Minix RV64 for RISC-V 64-bit"
	@echo "Usage:"
	@echo "  make [BOARD=<board>] [target]"
	@echo ""
	@echo "Boards:"
	@echo "  milkv-duo  - MilkV Duo CV1800B"
	@echo "  qemu-virt  - QEMU virt machine (default)"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build kernel image"
	@echo "  clean      - Remove build files"
	@echo "  qemu       - Run in QEMU"
	@echo "  qemu-debug - Run in QEMU with debug output"
	@echo "  qemu-gdb   - Run in QEMU and wait for GDB"
	@echo "  gdb        - Connect GDB to running QEMU"