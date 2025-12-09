# Minix RV64 QEMU 测试指南

## 快速开始

### 1. 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install qemu-system-riscv64

# Fedora/CentOS
sudo dnf install qemu-system-riscv64

# 或 macOS (使用 Homebrew)
brew install qemu
```

### 2. 安装交叉编译工具链

```bash
./tools/setup_toolchain.sh
```

### 3. 编译和运行

```bash
# 方法1: 使用测试脚本（推荐）
./scripts/test_qemu.sh

# 方法2: 手动编译
make BOARD=qemu-virt CROSS_COMPILE=riscv64-unknown-elf- all

# 运行 QEMU
make qemu

# 调试模式
make qemu-debug
```

## QEMU 命令详解

### 基本运行

```bash
qemu-system-riscv64 \
    -machine virt \
    -cpu rv64 \
    -nographic \
    -bios none \
    -kernel minix-rv64.elf
```

### 调试选项

```bash
# 启用 GDB 服务器
qemu-system-riscv64 ... -s -S

# 连接 GDB
riscv64-unknown-elf-gdb minix-rv64.elf
(gdb) target remote localhost:1234
(gdb) break kinit
(gdb) continue
```

### 网络支持

```bash
# 启用网络（已配置在 Makefile 中）
qemu-system-riscv64 ... \
    -device virtio-net-device,netdev=net0 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22
```

## 内存布局（QEMU virt）

| 地址范围 | 用途 |
|---------|------|
| 0x20000000 | Flash (可选) |
| 0x80000000 | RAM (内核加载) |
| 0x10000000 | UART0 |
| 0x0c000000 | PLIC (中断控制器) |
| 0x02000000 | CLINT (本地中断器) |

## 预期输出

成功启动后，应该看到类似输出：

```
=== Minix RV64 Booting ===
Board: QEMU RISC-V Virt
Kernel loaded at: 0x80000000
BSS cleared, size: 12345 bytes
Initializing kernel subsystems...
✓ Trap handling initialized
✓ Memory management initialized
✓ Scheduler initialized
✓ Drivers initialized
✓ Interrupts enabled
Entering kernel main loop...
```

## 故障排除

### 1. 编译错误

**错误**: `riscv64-unknown-elf-gcc: command not found`
```bash
# 解决方案：安装工具链
./tools/setup_toolchain.sh
source ~/.bashrc
```

**错误**: `undefined reference to 'xxx'`
- 检查是否所有源文件都添加到 Makefile
- 查看链接器脚本是否正确

### 2. 运行时错误

**问题**: QEMU 启动后立即退出
- 检查内核入口点是否正确
- 确认使用了正确的链接器脚本

**问题**: 没有串口输出
- 检查 UART 初始化
- 确认 UART 基地址正确
- 查看是否启用了 EARLY_PRINTK

### 3. 调试技巧

```bash
# 查看生成的汇编
riscv64-unknown-elf-objdump -d minix-rv64.elf > kernel.asm

# 查看符号表
riscv64-unknown-elf-nm minix-rv64.elf

# 查看内核头信息
riscv64-unknown-elf-readelf -h minix-rv64.elf
```

## 开发工作流

1. **修改代码**
   ```bash
   # 编辑源文件
   vim arch/riscv64/kernel/main.c
   ```

2. **编译**
   ```bash
   make clean && make
   ```

3. **测试**
   ```bash
   make qemu
   ```

4. **调试（如需要）**
   ```bash
   # 终端1
   make qemu-gdb

   # 终端2
   make gdb
   ```

## 扩展开发

### 添加新设备

1. 在 `include/board/qemu-virt.h` 中定义寄存器
2. 创建驱动文件在 `drivers/`
3. 在 `board_init()` 中初始化设备

### 添加系统调用

1. 在 `include/minix/syscall.h` 中定义
2. 在 `kernel/syscalls.c` 中实现
3. 在 `arch/riscv/kernel/trap.c` 中处理

### 内存管理扩展

1. 完善 `mm/page_alloc.c`
2. 实现 `mm/vm.c` 虚拟内存管理
3. 添加 slab 分配器

## 参考资源

- [QEMU RISC-V 文档](https://www.qemu.org/docs/master/system/riscv/virt.html)
- [RISC-V 规范](https://riscv.org/technical/specifications/)
- [QEMU 命令行选项](https://www.qemu.org/docs/master/system/index.html)