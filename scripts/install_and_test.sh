#!/bin/bash

# Installation and test script (requires root access)

echo "=== Minix RV64 Installation and Test ==="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "This script must be run as root"
    echo "Please run: sudo ./install_and_test.sh"
    exit 1
fi

# Install dependencies
echo "Installing dependencies..."
apt-get update
apt-get install -y \
    build-essential \
    bison \
    flex \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    texinfo \
    libisl-dev \
    autoconf \
    libtool \
    curl \
    wget \
    git \
    gcc-riscv64-linux-gnu

# Set environment variables
export CROSS_COMPILE=riscv64-linux-gnu-

# Build the kernel
echo ""
echo "Building Minix RV64..."
cd /home/donz/MinixRV64

# Clean previous builds
make clean

# Build for QEMU
make BOARD=qemu-virt CROSS_COMPILE=$CROSS_COMPILE all

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"

    # Show build artifacts
    echo ""
    echo "Build artifacts:"
    ls -la minix-rv64.*

    # Run QEMU test
    echo ""
    echo "=== Starting QEMU Test ==="
    echo "Press Ctrl+A, X to exit QEMU"
    echo ""

    # Run in background for a few seconds to capture output
    timeout 10s make qemu || true

else
    echo "✗ Build failed!"
    exit 1
fi

echo ""
echo "=== Test Complete ==="