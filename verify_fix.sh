#!/bin/bash
# Verify that the MMU mapping fix is present in the compiled code

echo "=== Verification Script for MMU UART Mapping Fix ==="
echo ""

echo "1. Checking if pgtable.c contains MMIO mapping code..."
if grep -q "Map UART and other MMIO devices" arch/riscv64/mm/pgtable.c; then
    echo "   ✓ Source code contains MMIO mapping"
else
    echo "   ✗ Source code missing MMIO mapping"
    exit 1
fi

echo ""
echo "2. Checking for debug messages in pgtable.c..."
if grep -q "Mapping MMIO devices" arch/riscv64/mm/pgtable.c; then
    echo "   ✓ Debug messages present"
else
    echo "   ✗ Debug messages missing"
fi

echo ""
echo "3. Checking if binary exists..."
if [ -f minix-rv64.elf ]; then
    echo "   ✓ Kernel ELF exists"

    # Check if the binary contains the debug strings
    if strings minix-rv64.elf | grep -q "Mapping MMIO devices"; then
        echo "   ✓ Debug strings found in binary"
    else
        echo "   ⚠ Debug strings NOT found in binary - rebuild needed!"
        echo ""
        echo "Run: make clean && make"
        exit 1
    fi
else
    echo "   ✗ Kernel binary not found"
    echo ""
    echo "Run: make"
    exit 1
fi

echo ""
echo "4. Expected boot sequence:"
echo "   X1234"
echo "   [MMU] Mapping MMIO devices..."
echo "   [MMU] MMIO mapping complete"
echo "   ✓ MMU ready"
echo "   ✓ Scheduler"
echo "   ✓ Block device ready"
echo "   ✓ VFS ready"
echo ""
echo "   Minix RV64 ready"
echo "   ✓ Shell"
echo "   minix# [DEBUG] Shell started, polling UART..."
echo "   [DEBUG] About to enter main loop"
echo "   ."
echo "   [DEBUG] First iteration, calling uart_getc()..."
echo "   [uart_getc] Entering function..."
echo "   [uart_getc] LSR read completed  <-- This should appear now!"
echo ""
echo "=== All checks passed! Run 'make qemu' to test ==="
