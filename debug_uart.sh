#!/bin/bash

echo "=========================================="
echo "UART调试测试"
echo "=========================================="
echo ""

# 创建测试输入
cat > /tmp/uart_test_input.txt << 'EOF'
h
e
l
p
EOF

echo "将发送字符: h e l p"
echo ""
echo "启动QEMU..."
echo ""

timeout 10s qemu-system-riscv64 \
    -machine virt \
    -cpu rv64 \
    -smp 1 \
    -m 128M \
    -bios none \
    -kernel minix-rv64.elf \
    -nographic \
    -serial mon:stdio \
    -d int,cpu_reset 2>&1 | tee /tmp/qemu_debug.log

echo ""
echo "=========================================="
echo "调试日志保存在: /tmp/qemu_debug.log"
echo "=========================================="
