#!/bin/bash

echo "=========================================="
echo "MinixRV64 Quick Interactive Test"
echo "=========================================="
echo ""
echo "Instructions:"
echo "1. QEMU will start"
echo "2. Wait for 'minix#' prompt"
echo "3. Try typing: help"
echo "4. Try typing: echo test"
echo "5. Press Ctrl+A then X to exit"
echo ""
echo "Starting in 3 seconds..."
sleep 3

qemu-system-riscv64 \
    -machine virt \
    -cpu rv64 \
    -smp 1 \
    -m 128M \
    -bios none \
    -kernel minix-rv64.elf \
    -serial mon:stdio \
    -nographic \
    -device virtio-net-device,netdev=net0 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22
