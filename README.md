# Minix RV64 for MilkV Duo CV1800B

## 目标
将 Minix 操作系统移植到 RISC-V 64位架构，特别针对 MilkV Duo CV1800B 开发板。

**最新更新**: 2025-12-10 - 完善了文件系统和UART驱动，详见 [IMPROVEMENTS_SUMMARY.md](IMPROVEMENTS_SUMMARY.md)

## 硬件平台
- **开发板**: MilkV Duo
- **芯片**: CV1800B (Sophon SG2000)
- **CPU**: T-Head XuanTie C906 (RISC-V 64位, 1GHz)
- **内存**: 512MB/1GB LPDDR4
- **外设**: UART, GPIO, I2C, SPI, USB 2.0

## 项目结构
```
MinixRV64/
├── arch/riscv64/     # RISC-V 64位架构相关代码
├── boot/            # 引导加载程序
├── crt/             # C运行时
├── drivers/         # 设备驱动
├── fs/              # 文件系统
├── include/         # 头文件
├── init/            # 初始化代码
├── kernel/          # 内核核心
├── lib/             # 库函数
├── mm/              # 内存管理
├── net/             # 网络栈
└── tools/           # 开发工具
```

## 功能特性

### ✓ 已实现
- 基础架构和引导
- 内存管理 (页分配器, Slab分配器)
- 进程调度框架
- **UART驱动** (缓冲区, 中断支持)
- **VFS虚拟文件系统** (路径解析, 挂载管理)
- **devfs设备文件系统**
- **ramfs内存文件系统**
- 块设备接口

### ⚠ 进行中
- 完整的进程管理
- 系统调用实现
- EXT2/FAT32文件系统

### ☐ 计划中
- GPIO驱动
- SD卡驱动
- 网络协议栈
- 用户态程序

## 开发阶段
1. **阶段1**: 基础架构和引导 ✓
2. **阶段2**: 内存管理和进程 ✓
3. **阶段3**: 设备驱动 (进行中)
4. **阶段4**: 文件系统 (进行中)
5. **阶段5**: 网络和应用 (计划中)

## 快速开始

### 编译
```bash
make clean
make BOARD=qemu-virt
```

### 在QEMU中运行
```bash
make qemu
```

### 调试
```bash
make qemu-gdb    # 终端1
make gdb         # 终端2
```

## 交叉编译工具链
- 目标: riscv64-unknown-elf
- GCC 13+ with RV64GC support

## 文档
- [ARCHITECTURE.md](ARCHITECTURE.md) - 系统架构概览
- [FILESYSTEM.md](FILESYSTEM.md) - 文件系统设计
- [UART_DRIVER.md](UART_DRIVER.md) - UART驱动文档
- [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) - 项目结构
- [IMPROVEMENTS_SUMMARY.md](IMPROVEMENTS_SUMMARY.md) - 最新更新总结

## 内存映射 (示例)
- 0x00000000: Boot ROM
- 0x80000000: RAM start
- 0x80100000: Kernel load
- 0x80200000: Device tree