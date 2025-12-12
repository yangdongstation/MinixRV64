#!/bin/bash

# Test Minix RV64 with OpenSBI firmware

echo "=== Testing Minix RV64 with OpenSBI ==="
echo ""

# Check if OpenSBI firmware is available
OPENSBI_DEFAULT="/usr/share/qemu-riscv64/opensbi-riscv64-generic-fw_dynamic.bin"

if [ -f "$OPENSBI_DEFAULT" ]; then
    echo "✓ Found OpenSBI firmware at $OPENSBI_DEFAULT"
    BIOS_FILE="$OPENSBI_DEFAULT"
elif [ -f "/usr/share/qemu/opensbi-riscv64-generic-fw_dynamic.bin" ]; then
    echo "✓ Found OpenSBI firmware at /usr/share/qemu/opensbi-riscv64-generic-fw_dynamic.bin"
    BIOS_FILE="/usr/share/qemu/opensbi-riscv64-generic-fw_dynamic.bin"
else
    echo "✗ OpenSBI firmware not found"
    echo ""
    echo "You can install it with:"
    echo "  Ubuntu/Debian: sudo apt-get install opensbi-qemu"
    echo "  Fedora:       sudo dnf install opensbi"
    exit 1
fi

echo ""
echo "Running QEMU with OpenSBI..."
echo "Command: qemu-system-riscv64 -machine virt -bios $BIOS_FILE -kernel minix-rv64.elf"
echo ""

# Run QEMU with OpenSBI
timeout 10s qemu-system-riscv64 -machine virt \
    -cpu rv64 -smp 1 -m 128M \
    -bios "$BIOS_FILE" \
    -kernel minix-rv64.elf \
    -nographic -no-reboot 2>&1 || true

echo ""
echo "Test complete"