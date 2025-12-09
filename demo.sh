#!/bin/bash

# Demo script showing how to run Minix RV64 on QEMU

echo "=== Minix RV64 QEMU Demo ==="
echo ""

# Check QEMU
echo "✓ QEMU RISC-V 64-bit found at: $(which qemu-system-riscv64)"
echo "  Version: $(qemu-system-riscv64 -version 2>&1 | head -1)"
echo ""

# Show what the QEMU command would be
echo "To run Minix RV64 on QEMU, you would use the following command:"
echo ""
echo "qemu-system-riscv64 \\"
echo "    -machine virt \\"
echo "    -cpu rv64 \\"
echo "    -nographic \\"
echo "    -bios none \\"
echo "    -kernel minix-rv64.elf \\"
echo "    -smp 1 \\"
echo "    -m 128M"
echo ""

# Explain each parameter
echo "Parameters explained:"
echo "  -machine virt    : Use QEMU's virt machine for RISC-V"
echo "  -cpu rv64        : Emulate a RISC-V 64-bit CPU"
echo "  -nographic       : Use serial console instead of graphic display"
echo "  -bios none       : Don't load BIOS, we load kernel directly"
echo "  -kernel minix-rv64.elf : Load our kernel at 0x80000000"
echo "  -smp 1           : Single core CPU"
echo "  -m 128M          : 128MB RAM"
echo ""

# Show debug version
echo "For debugging with GDB:"
echo ""
echo "qemu-system-riscv64 ... -s -S"
echo ""
echo "Then connect with:"
echo "riscv64-unknown-elf-gdb minix-rv64.elf"
echo "(gdb) target remote localhost:1234"
echo "(gdb) break kinit"
echo "(gdb) continue"
echo ""

# Memory map info
echo "Memory Map:"
echo "  0x80000000 - 0x80004000 : Kernel code/data"
echo "  0x10000000 - 0x10000100 : UART0 (for console output)"
echo "  0x0c000000 - 0x0c010000 : PLIC (Interrupt controller)"
echo "  0x02000000 - 0x02010000 : CLINT (Local interruptor)"
echo ""

echo "Note: This demo shows the commands. To actually run, you need to:"
echo "1. Install RISC-V toolchain (./tools/setup_toolchain.sh)"
echo "2. Compile the kernel (make)"
echo "3. Run QEMU (make qemu)"