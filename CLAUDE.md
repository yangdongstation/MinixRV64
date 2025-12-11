# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MinixRV64 is a port of the Minix operating system to RISC-V 64-bit architecture, specifically targeting the MilkV Duo CV1800B development board (T-Head XuanTie C906 CPU). The project implements a microkernel architecture with device drivers, virtual file system, memory management, and a basic shell.

**Current Status**: The system boots successfully in QEMU with full interactive shell support. UART input/output works correctly after fixing QEMU serial configuration.

## Build and Test Commands

### Essential Commands

```bash
# Build the kernel (default: QEMU target)
make clean && make

# Build for specific board
make BOARD=qemu-virt    # QEMU virt machine (default)
make BOARD=milkv-duo    # MilkV Duo CV1800B

# Run in QEMU (interactive shell works!)
make qemu

# Exit QEMU: Press Ctrl+A, then X

# Debug with GDB (requires two terminals)
make qemu-gdb    # Terminal 1: Start QEMU waiting for GDB
make gdb         # Terminal 2: Connect GDB to QEMU
```

### Shell Commands Available

When running `make qemu`, the system boots to a `minix#` prompt with these commands:
- `help` - Show all commands
- `echo` - Echo arguments
- `uname` - Show system information
- `pwd` - Print working directory
- `ls`, `cat`, `mkdir`, `rm` - File operations
- `ps`, `kill` - Process management
- `clear`, `reboot` - System commands

## Architecture Overview

### Memory Layout

```
0x00000000  Boot ROM
0x03000000  Peripheral registers (UART, GPIO, I2C, SPI, Timer, Interrupt Controller)
0x80000000  Kernel code (.text) ← Kernel load address
0x80100000  Kernel data (.rodata, .data, .bss)
0x80200000  Kernel stack
0x80300000  Kernel heap
0x81000000  User space (processes)
```

### Boot Sequence

```
arch/riscv64/boot/start.S (Assembly initialization)
  ↓
arch/riscv64/kernel/main.c:kernel_main()
  ↓ board_init()
  ↓ trap_init()
  ↓ mm_init()
  ↓ sched_init()
  ↓ drivers_init()
  ↓ shell_init()
  ↓
kernel/shell.c:shell_run() (main loop)
```

### Key Subsystems

**Memory Management** (`arch/riscv64/mm/`):
- `page_alloc.c` - Physical page allocator (buddy algorithm)
- `pgtable.c` - Page table management (SV39: 39-bit virtual addressing)
- `slab.c` - Kernel object allocator (slab allocator)
- `mmu.c` - MMU initialization and control

**File System** (`fs/`):
- `vfs.c` - Virtual File System layer with unified file interface, path resolution, mount management
- `devfs.c` - Device file system (character/block device nodes, dynamic registration)
- `ramfs.c` - In-memory file system (max file size 1MB)
- `ext2.c`, `fat32.c` - Disk file systems (partially implemented)

**Drivers** (`drivers/`):
- `char/uart.c` - 16550-compatible UART driver with circular buffers (256 bytes), interrupt support, configurable baud rate
- `block/blockdev.c` - Block device interface

**Process Management** (`kernel/`):
- `sched.c` - Preemptive priority scheduler
- `proc.c` - Process state management (ready, running, blocked, zombie)
- `syscalls.c` - System call interface (POSIX-compatible)

### Board Configuration

The build system supports multiple targets through `BOARD` variable:
- **qemu-virt**: QEMU's virt machine (UART base: 0x10000000)
- **milkv-duo**: MilkV Duo CV1800B (UART base: 0x04140000)

Board-specific configuration is in:
- `include/board/qemu-virt.h`
- `include/board/cv1800b.h`

The Makefile sets `-DBOARD=1` (milkv-duo) or `-DBOARD=2` (qemu-virt) which is used in code for conditional compilation.

## Critical Implementation Details

### UART Driver (FIXED)

**Important**: The QEMU serial configuration was the source of input problems. The Makefile previously used `QEMU_SERIAL = mon:stdio` which intercepted keyboard input for the QEMU monitor. The fix was to change it to `QEMU_SERIAL = stdio` (line 77 in Makefile). This allows characters to flow directly to the UART driver.

The UART driver implements:
- 32-bit register access (required for QEMU NS16550A compatibility)
- Circular RX/TX buffers (256 bytes each)
- Polling mode for input (`uart_getchar()` in shell loop)
- Direct register access at 0x10000000 for QEMU

### Shell Implementation

The shell (`kernel/shell.c`) runs as the main kernel loop:
1. Prints `minix#` prompt
2. Polls UART via `uart_getchar()` in tight loop
3. Buffers input until Enter/Return (handles backspace)
4. Parses command line into argc/argv
5. Executes command from command table
6. Returns to step 1

Commands are registered in `commands[]` array with function pointers.

### VFS Architecture

The VFS layer provides:
- Filesystem registration via `vfs_register_fs()`
- Mount point management (up to 32 mount points)
- Path resolution supporting `.` and `..`
- Inode cache with hash table (256 buckets)
- Unified file operations through `fs_ops_t` structure

File systems implement the `fs_ops_t` interface to integrate with VFS.

### Page Table Management

Uses RISC-V SV39 paging:
- 3-level page tables
- 39-bit virtual addresses
- 4KB page size
- Page table entries stored in `arch/riscv64/mm/pgtable.c`

Currently running with MMU initialized but in relatively simple mapping mode (see console output "MMU ready (DISABLED)").

## Development Patterns

### Adding a New Driver

1. Create source: `drivers/{char,block}/newdriver.c`
2. Create header: `include/minix/newdriver.h`
3. Implement initialization function
4. Register in `kernel/drivers.c:drivers_init()`
5. Add to Makefile `C_SRCS`
6. Rebuild: `make clean && make`

### Adding a New File System

1. Create `fs/newfs.c`
2. Implement `fs_ops_t` interface (mount, unmount, lookup, read, write, etc.)
3. Register via `vfs_register_fs()` in initialization code
4. Add to Makefile `C_SRCS`
5. Test mount operations through VFS

### Adding a Shell Command

1. Add function prototype in `include/minix/shell.h`
2. Implement `int cmd_newcmd(int argc, char **argv)` in `kernel/shell.c`
3. Add entry to `commands[]` table: `{"newcmd", "Description", cmd_newcmd}`
4. Rebuild and test with `make qemu`

## Code Style and Conventions

- **Naming**:
  - Functions and variables: `snake_case`
  - Types: `snake_case_t`
  - Macros and constants: `UPPER_CASE`
- **Indentation**: Tabs (8 spaces) following Linux kernel style
- **Files**: C sources are `.c`, assembly sources are `.S` (preprocessed) or `.s` (raw)
- **Headers**: Include guards use `_PATH_FILENAME_H` pattern

## Common Pitfalls

1. **UART Register Access**: Must use 32-bit access (`volatile unsigned int *`) not 8-bit. QEMU's NS16550A requires this.

2. **Serial Configuration**: Never use `mon:stdio` for QEMU serial - it breaks input. Use `stdio` only.

3. **MMU/Virtual Memory**: The system currently runs with simple memory mapping. Be careful when modifying page tables or enabling full virtual memory.

4. **Interrupt Handling**: Shell currently uses polling for UART input. When adding interrupt-driven I/O, ensure proper synchronization.

5. **Board Selection**: Always specify `BOARD=qemu-virt` or `BOARD=milkv-duo`. Default is qemu-virt. This affects UART base addresses and peripheral configuration.

6. **Linker Scripts**: Different boards use different linker scripts (`kernel.ld` vs `kernel_qemu.ld`). Make sure modifications are applied to the correct one.

## Documentation Files

The repository contains extensive documentation (primarily in Chinese):
- `README.md` - Main project overview and status
- `ARCHITECTURE.md` - System architecture and design
- `PROJECT_STRUCTURE.md` - Detailed directory structure and module descriptions
- `FILESYSTEM.md` - File system design and implementation
- `UART_DRIVER.md` - UART driver details
- `QUICK_START.md` - Quick start guide
- `INPUT_PROBLEM_SOLVED.md` - Documents the UART input fix

When making significant changes, update relevant documentation files.

## Toolchain

- **Compiler**: `riscv64-unknown-elf-gcc` (GCC 13+ recommended)
- **Architecture**: RV64GC (RV64IMAFDC)
- **ABI**: lp64d (64-bit long/pointer, double-precision float)
- **Code Model**: medany (position-independent code within ±2GB)
- **Flags**: `-nostdlib -fno-builtin` (freestanding environment)

## Testing Workflow

1. Make changes to source files
2. `make clean && make BOARD=qemu-virt`
3. `make qemu`
4. Test at the `minix#` prompt
5. Use `Ctrl+A X` to exit QEMU
6. Repeat as needed

For debugging:
1. Terminal 1: `make qemu-gdb` (starts QEMU with `-s -S` waiting for GDB)
2. Terminal 2: `make gdb` (connects GDB, layout split view)
3. Set breakpoints, step through code
4. QEMU serial output appears in Terminal 1

## Project Status Reference

✅ **Working**: Kernel boot, MMU init, scheduler framework, UART I/O (input/output), interactive shell, VFS layer, devfs, ramfs, block device interface

⚠️ **Partial**: Process management, system calls, EXT2/FAT32 implementations

☐ **TODO**: GPIO driver, SD card driver, full process model, networking, user-space programs
