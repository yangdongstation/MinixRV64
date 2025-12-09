#!/bin/bash
# Interactive test for UART input

echo "Testing UART input interactively..."
echo "This will send 'help' and 'ps' commands to test character repetition fix."
echo ""

# Use expect-like behavior with timeout
(
    sleep 2  # Wait for boot
    echo "help"
    sleep 2
    echo "ps"
    sleep 2
    echo "echo test"
    sleep 2
) | timeout 20 qemu-system-riscv64 -M virt -bios none -kernel minix-rv64.elf -nographic -serial mon:stdio 2>&1
