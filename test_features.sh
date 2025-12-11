#!/bin/bash

# MinixRV64 Feature Test Script
# Tests UART and filesystem functionality

echo "=========================================="
echo "MinixRV64 Feature Test"
echo "=========================================="
echo ""

# Check if binary exists
if [ ! -f minix-rv64.elf ]; then
    echo "Error: minix-rv64.elf not found!"
    echo "Please run 'make' first."
    exit 1
fi

echo "✓ Binary found: minix-rv64.elf"
echo ""

# Show binary info
echo "Binary Information:"
echo "-------------------"
ls -lh minix-rv64.elf minix-rv64.bin
echo ""

# Show symbols related to our new features
echo "Checking for new features in binary:"
echo "-------------------------------------"
echo "UART functions:"
riscv64-unknown-elf-nm minix-rv64.elf | grep uart_ | head -10
echo ""

echo "VFS functions:"
riscv64-unknown-elf-nm minix-rv64.elf | grep vfs_ | head -10
echo ""

echo "Filesystem functions:"
riscv64-unknown-elf-nm minix-rv64.elf | grep -E 'devfs|ramfs' | head -10
echo ""

# Run in QEMU with timeout
echo "=========================================="
echo "Running in QEMU (5 second test)..."
echo "=========================================="
echo ""

timeout 5s qemu-system-riscv64 \
    -machine virt \
    -cpu rv64 \
    -smp 1 \
    -m 128M \
    -bios none \
    -kernel minix-rv64.elf \
    -serial mon:stdio \
    -nographic \
    -device virtio-net-device,netdev=net0 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 2>&1 | tee /tmp/minix_test.log

echo ""
echo "=========================================="
echo "Test Complete"
echo "=========================================="
echo ""
echo "Output saved to: /tmp/minix_test.log"
echo ""
echo "To run interactively: make qemu"
