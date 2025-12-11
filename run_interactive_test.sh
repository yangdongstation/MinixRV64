#!/bin/bash
# Run QEMU and send a test character after boot

echo "Starting QEMU..."
echo "System should boot and you'll see the shell prompt."
echo "After 3 seconds, we'll send the letter 'a' to test input."
echo ""

# Run QEMU in background, send 'a' after delay, then wait
(sleep 3 && echo "a") | timeout 8 qemu-system-riscv64 \
    -machine virt \
    -cpu rv64 \
    -smp 1 \
    -m 128M \
    -bios none \
    -kernel minix-rv64.elf \
    -serial stdio \
    -nographic \
    -monitor none
