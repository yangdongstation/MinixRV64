#!/bin/bash
# Test script for UART input

echo "Testing UART input..."
echo "Commands will be sent automatically to test if input works."
echo ""

# Send commands to QEMU via stdin
(
    sleep 2  # Wait for boot
    echo "help"
    sleep 1
    echo "uname"
    sleep 1
    echo "echo hello world"
    sleep 1
) | timeout 10 qemu-system-riscv64 -M virt -bios none -kernel minix-rv64.elf -nographic -serial mon:stdio 2>&1
