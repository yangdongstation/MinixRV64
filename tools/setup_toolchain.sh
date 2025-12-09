#!/bin/bash

# Setup RISC-V 64-bit cross-compilation toolchain for Minix RV64

echo "Setting up RISC-V 64-bit toolchain..."

# Check if toolchain is already installed
if command -v riscv64-unknown-elf-gcc &> /dev/null; then
    echo "Toolchain already found!"
    riscv64-unknown-elf-gcc --version
    exit 0
fi

# Install prerequisites
echo "Installing prerequisites..."
sudo apt-get update
sudo apt-get install -y \
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
    git

# Download pre-built toolchain
TOOLCHAIN_VERSION="13.2.0-1"
TOOLCHAIN_DIR="/opt/riscv64"
TOOLCHAIN_URL="https://github.com/riscv-collab/riscv-gnu-toolchain/releases/download/2024.04.12/riscv64-unknown-elf-gcc-${TOOLCHAIN_VERSION}-linux-x86_64.tar.gz"

echo "Downloading RISC-V toolchain..."
mkdir -p $TOOLCHAIN_DIR
cd $TOOLCHAIN_DIR

if [ ! -f "toolchain.tar.gz" ]; then
    wget -O toolchain.tar.gz $TOOLCHAIN_URL
fi

echo "Extracting toolchain..."
sudo tar -xzf toolchain.tar.gz --strip-components=1

# Add to PATH
if ! grep -q "$TOOLCHAIN_DIR/bin" ~/.bashrc; then
    echo "export PATH=\$PATH:$TOOLCHAIN_DIR/bin" >> ~/.bashrc
fi

# Update current session
export PATH=$PATH:$TOOLCHAIN_DIR/bin

# Verify installation
echo "Verifying toolchain installation..."
riscv64-unknown-elf-gcc --version

if [ $? -eq 0 ]; then
    echo "Toolchain setup complete!"
    echo ""
    echo "Usage:"
    echo "  make CROSS_COMPILE=riscv64-unknown-elf-"
    echo ""
    echo "Note: Run 'source ~/.bashrc' to permanently add toolchain to PATH"
else
    echo "Toolchain installation failed!"
    exit 1
fi