#!/bin/bash

# Test script for Minix RV64 on QEMU

echo "=== Minix RV64 QEMU Test Script ==="
echo ""

# Check if QEMU is installed
if ! command -v qemu-system-riscv64 &> /dev/null; then
    echo "ERROR: QEMU for RISC-V is not installed!"
    echo "Install with:"
    echo "  Ubuntu/Debian: sudo apt-get install qemu-system-riscv64"
    echo "  Fedora:       sudo dnf install qemu-system-riscv"
    exit 1
fi

# Check if toolchain is available
if ! command -v riscv64-unknown-elf-gcc &> /dev/null; then
    echo "WARNING: RISC-V toolchain not found in PATH!"
    echo "Run ./tools/setup_toolchain.sh to install it"
    echo ""
fi

# Set board to QEMU
export BOARD=qemu-virt

# Clean previous build
echo "Cleaning previous build..."
make clean 2>/dev/null

# Build for QEMU
echo ""
echo "Building Minix RV64 for QEMU..."
if make CROSS_COMPILE=riscv64-unknown-elf- all; then
    echo "✓ Build successful!"
else
    echo "✗ Build failed!"
    echo ""
    echo "Possible solutions:"
    echo "1. Install RISC-V toolchain: ./tools/setup_toolchain.sh"
    echo "2. Check if riscv64-unknown-elf-gcc is in PATH"
    echo "3. Verify all dependencies are installed"
    exit 1
fi

# Show kernel information
echo ""
echo "Kernel image information:"
ls -la minix-rv64.*
echo ""
echo "Kernel ELF header:"
if command -v riscv64-unknown-elf-readelf &> /dev/null; then
    riscv64-unknown-elf-readelf -h minix-rv64.elf | grep "Class\|Machine\|Entry"
fi

# Ask to run QEMU
echo ""
read -p "Run QEMU now? (y/N): " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Starting QEMU (press Ctrl+A, X to exit)..."
    echo ""
    make qemu
else
    echo ""
    echo "To run QEMU manually:"
    echo "  make qemu            - Run with serial output"
    echo "  make qemu-debug      - Run with debug"
    echo "  make qemu-gdb        - Run and wait for GDB"
    echo "  make gdb             - Connect GDB to running QEMU"
fi

echo ""
echo "=== Test complete ==="