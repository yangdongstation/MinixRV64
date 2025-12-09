# Minix RV64 for MilkV Duo CV1800B

## 目标
将 Minix 操作系统移植到 RISC-V 64位架构，特别针对 MilkV Duo CV1800B 开发板。

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

## 开发阶段
1. **阶段1**: 基础架构和引导
2. **阶段2**: 内存管理和进程
3. **阶段3**: 设备驱动
4. **阶段4**: 文件系统
5. **阶段5**: 网络和应用

## 交叉编译工具链
- 目标: riscv64-unknown-elf
- GCC 13+ with RV64GC support

## 内存映射 (示例)
- 0x00000000: Boot ROM
- 0x80000000: RAM start
- 0x80100000: Kernel load
- 0x80200000: Device tree