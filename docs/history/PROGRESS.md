# Minix RV64 移植进度报告

## 项目概述

成功搭建了 Minix 操作系统到 RISC-V 64位架构的移植框架，支持 MilkV Duo CV1800B 和 QEMU virt 两个目标平台。

## 已完成的工作

### ✅ 1. 项目架构设计
- [x] 创建了标准的 Linux/Minix 风格目录结构
- [x] 设计了多平台支持的架构（MilkV Duo + QEMU）
- [x] 创建了详细的架构文档 (ARCHITECTURE.md)

### ✅ 2. 构建系统
- [x] 支持交叉编译的 Makefile
- [x] 多板卡支持（通过 BOARD 参数）
- [x] 自动化构建脚本 (build.sh)
- [x] 工具链安装脚本 (tools/setup_toolchain.sh)

### ✅ 3. 启动代码
- [x] RISC-V 汇编启动代码 (arch/riscv64/boot/start.S)
- [x] 机器模式切换到监督模式
- [x] 栈和BSS段初始化
- [x] C语言入口点 (kinit)

### ✅ 4. 硬件抽象层
- [x] CV1800B 硬件定义 (include/board/cv1800b.h)
- [x] QEMU virt 硬件定义 (include/board/qemu-virt.h)
- [x] 统一的板卡配置系统 (include/minix/board.h)
- [x] CSR寄存器定义 (include/asm/csr.h)
- [x] I/O操作抽象 (include/asm/io.h)

### ✅ 5. 驱动程序
- [x] 通用UART驱动（支持16550A兼容）
- [x] 调试串口输出
- [x] 多板卡UART配置支持

### ✅ 6. 中断和异常处理
- [x] 中断处理框架 (arch/riscv64/kernel/trap.c)
- [x] 异常向量表
- [x] 上下文保存/恢复
- [x] 系统调用入口

### ✅ 7. 核心系统组件
- [x] 进程调度器骨架 (kernel/sched.c)
- [x] 系统调用接口 (kernel/syscalls.c)
- [x] 内存管理基础 (arch/riscv64/mm/mmu.c)
- [x] 板卡特定初始化 (kernel/board.c)

### ✅ 8. 调试支持
- [x] printk/printf 实现 (lib/printk.c)
- [x] QEMU调试支持
- [x] GDB集成
- [x] 详细的调试文档

### ✅ 9. 测试框架
- [x] 构建测试脚本 (tests/build_test.sh)
- [x] QEMU测试脚本 (scripts/test_qemu.sh)
- [x] 项目完整性检查

## 项目结构

```
MinixRV64/
├── arch/riscv64/          # RISC-V架构代码
│   ├── boot/             # 启动代码
│   ├── kernel/           # 内核核心
│   ├── mm/               # 内存管理
│   └── include/          # 头文件
├── drivers/              # 设备驱动
│   └── char/uart.c       # UART驱动
├── include/              # 公共头文件
│   ├── minix/            # Minix定义
│   ├── asm/              # 汇编定义
│   └── board/            # 板卡定义
├── kernel/               # 通用内核代码
│   ├── sched.c           # 调度器
│   ├── syscalls.c        # 系统调用
│   └── board.c           # 板卡支持
├── lib/                  # 库函数
│   └── printk.c          # 打印函数
├── tools/                # 工具
│   └── setup_toolchain.sh
├── scripts/              # 脚本
│   └── test_qemu.sh
├── tests/                # 测试
│   └── build_test.sh
└── build.sh              # 主构建脚本
```

## 使用方法

### 安装工具链
```bash
./tools/setup_toolchain.sh
```

### 编译（QEMU）
```bash
# 方法1：使用构建脚本
./build.sh

# 方法2：使用 Makefile
make BOARD=qemu-virt CROSS_COMPILE=riscv64-unknown-elf-
```

### 运行 QEMU
```bash
make qemu
```

### 编译（MilkV Duo）
```bash
make BOARD=milkv-duo CROSS_COMPILE=riscv64-unknown-elf-
```

## 技术特点

1. **多平台支持**: 统一代码支持 QEMU 和实际硬件
2. **标准 RISC-V**: 使用标准的 RISC-V 64位 RV64GC 指令集
3. **Minix 兼容**: 保留 Minix 的微内核特性
4. **模块化设计**: 清晰的模块分离，便于维护和扩展
5. **完整的工具链**: 提供完整的开发和调试工具

## 下一步工作

### 1. 内存管理（优先级：高）
- [ ] 实现物理页分配器
- [ ] 实现虚拟内存管理（SV39页表）
- [ ] 实现 slab 分配器
- [ ] 添加内存保护

### 2. 进程管理（优先级：高）
- [ ] 完善进程调度算法
- [ ] 实现进程创建和销毁
- [ ] 添加上下文切换
- [ ] 实现进程间通信

### 3. 设备驱动（优先级：中）
- [ ] GPIO驱动
- [ ] 定时器驱动
- [ ] 中断控制器驱动
- [ ] 存储设备驱动（SD/eMMC）

### 4. 文件系统（优先级：中）
- [ ] 实现Minix文件系统
- [ ] 添加VFS层
- [ ] 支持设备文件
- [ ] 添加mount/umount功能

### 5. 网络栈（优先级：低）
- [ ] 基础TCP/IP协议栈
- [ ] VirtIO网络驱动
- [ ] Socket API实现
- [ ] 网络应用程序支持

## 预期结果

当前代码已经可以：
1. 成功编译为 RISC-V 64位 ELF 文件
2. 在 QEMU 中启动并输出调试信息
3. 通过串口进行交互
4. 支持基本的异常和中断处理

## 贡献指南

1. 遵循现有的代码风格
2. 为新功能添加测试
3. 更新相关文档
4. 提交前运行完整测试

## 参考资料

- [RISC-V 规范](https://riscv.org/technical/specifications/)
- [Minix 3 文档](http://www.minix3.org/)
- [QEMU RISC-V 文档](https://www.qemu.org/docs/master/system/riscv/virt.html)
- [MilkV Duo 技术文档](https://milkv.io/docs/duo)