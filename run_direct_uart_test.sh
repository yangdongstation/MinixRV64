#!/bin/bash

echo "=========================================="
echo "直接UART硬件测试"
echo "=========================================="
echo ""
echo "这个测试将："
echo "1. 直接读取UART LSR寄存器"
echo "2. 显示LSR的所有变化"
echo "3. 如果检测到输入，显示字符"
echo ""
echo "请在QEMU启动后尝试输入字符！"
echo ""

# 编译测试程序
echo "编译测试程序..."
riscv64-unknown-elf-gcc -march=rv64gc -mabi=lp64d -mcmodel=medany \
    -nostdlib -fno-builtin \
    -O0 -g \
    -Ttext=0x80000000 \
    -o test_direct_uart test_direct_uart.c

if [ $? -ne 0 ]; then
    echo "编译失败！"
    exit 1
fi

echo "编译成功！"
echo ""
echo "启动QEMU..."
echo ""
echo "提示："
echo "- LSR的bit 0 (值0x01) = 数据就绪"
echo "- LSR的bit 5 (值0x20) = 发送器空"
echo "- 正常LSR应该是0x60 (发送器空 + 发送保持寄存器空)"
echo "- 如果有输入，LSR应该变成0x61"
echo ""
echo "现在尝试输入字符..."
echo ""

# 运行QEMU
timeout 30s qemu-system-riscv64 \
    -machine virt \
    -cpu rv64 \
    -smp 1 \
    -m 128M \
    -bios none \
    -kernel test_direct_uart \
    -nographic \
    -serial stdio \
    -monitor none

echo ""
echo "=========================================="
echo "测试结束"
echo "=========================================="
