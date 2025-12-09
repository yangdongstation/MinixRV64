#!/bin/bash

# Quick build test without requiring toolchain

echo "=== Minix RV64 Build Test ==="
echo ""

# Check if required source files exist
echo "Checking source files..."

REQUIRED_FILES=(
    "arch/riscv64/boot/start.S"
    "arch/riscv64/kernel/main.c"
    "arch/riscv64/kernel/trap.c"
    "arch/riscv64/kernel/trap.S"
    "arch/riscv64/mm/mmu.c"
    "drivers/char/uart.c"
    "kernel/sched.c"
    "kernel/syscalls.c"
    "kernel/board.c"
    "lib/printk.c"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "✗ $file - MISSING!"
    fi
done

echo ""
echo "Checking header files..."

HEADER_FILES=(
    "include/minix/config.h"
    "include/minix/board.h"
    "include/minix/print.h"
    "include/minix/syscall.h"
    "include/types.h"
    "include/asm/csr.h"
    "include/asm/io.h"
    "include/board/qemu-virt.h"
    "include/board/cv1800b.h"
)

for file in "${HEADER_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "✗ $file - MISSING!"
    fi
done

echo ""
echo "Checking Makefiles..."

MAKE_FILES=(
    "Makefile"
    "arch/riscv64/kernel.ld"
    "arch/riscv64/kernel_qemu.ld"
)

for file in "${MAKE_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "✗ $file - MISSING!"
    fi
done

echo ""
echo "Checking scripts..."

SCRIPT_FILES=(
    "tools/setup_toolchain.sh"
    "scripts/test_qemu.sh"
    "build.sh"
)

for file in "${SCRIPT_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
        if [ -x "$file" ]; then
            echo "  → Executable"
        else
            echo "  → Not executable"
        fi
    else
        echo "✗ $file - MISSING!"
    fi
done

echo ""
echo "Project structure check complete!"

# Test make syntax
echo ""
echo "Testing Makefile syntax..."
if make -n >/dev/null 2>&1; then
    echo "✓ Makefile syntax is valid"
else
    echo "✗ Makefile has syntax errors"
fi

echo ""
echo "Build test complete!"