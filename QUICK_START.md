# MinixRV64 Quick Start Guide

## TL;DR - 快速运行

```bash
make qemu
# 等待 "minix#" 提示符
# 输入: help
# 按 Ctrl+A 然后 X 退出
```

---

## 编译与运行

```bash
make clean    # 清理旧构建
make          # 编译内核
make qemu     # 在 QEMU 中运行
```

## 基本命令

```bash
minix# help        # 显示所有命令
minix# echo Hi!    # 测试 echo
minix# uname       # 系统信息
minix# pwd         # 当前目录
minix# ps          # 进程列表
minix# ls          # 列出文件
minix# mkdir test  # 创建目录
minix# cat file    # 查看文件
```

## 退出 QEMU

- 按 `Ctrl+A`
- 然后按 `X`

---

## 当前功能状态

### 已完成功能

| 功能 | 状态 | 说明 |
|------|------|------|
| 内核启动 | ✅ | RISC-V S-mode 运行 |
| UART I/O | ✅ | 输入输出正常 |
| 交互式 Shell | ✅ | 16 个命令可用 |
| VFS 文件系统 | ✅ | 路径解析、挂载管理 |
| ramfs | ✅ | 内存文件系统读写 |
| O(1) 调度器 | ✅ | 框架已实现 |
| 进程管理 | ✅ | task_struct、fork/exit 框架 |
| ELF 加载器 | ✅ | 验证和段加载框架 |

### 部分实现 (框架存在)

| 功能 | 状态 | 说明 |
|------|------|------|
| 内存管理 | ⚠️ | kmalloc 有 bug，使用静态数组 |
| 进程调度 | ⚠️ | 需要完整集成 |
| Fork/Exec | ⚠️ | 需要 COW 和 VFS 集成 |

### 未实现

- 完整虚拟内存
- 用户态程序运行
- 信号处理
- EXT2/FAT32 文件系统

---

## 调试

### 使用 GDB 调试

```bash
# 终端 1: 启动 QEMU 等待 GDB
make qemu-gdb

# 终端 2: 连接 GDB
make gdb
```

### 常用 GDB 命令

```gdb
b kernel_main     # 在 kernel_main 设断点
c                 # 继续执行
n                 # 单步执行
p variable        # 打印变量
bt                # 查看调用栈
```

---

## 预期输出

```
$ make qemu

✓ O(1) Scheduler
✓ Fork subsystem
Minix RV64 ready
✓ Shell
minix# help
Available commands:
  help    - Show this help
  echo    - Echo arguments
  clear   - Clear screen
  uname   - Show system info
  pwd     - Print working directory
  ls      - List directory
  cat     - Show file contents
  mkdir   - Create directory
  touch   - Create empty file
  write   - Write to file
  rm      - Remove file
  mount   - Mount filesystem
  ps      - Show processes
  kill    - Kill process
  mem     - Show memory info
  reboot  - Reboot system
minix#
```

---

## 项目结构

```
MinixRV64/
├── arch/riscv64/     # RISC-V 架构代码
│   ├── boot/         # 启动代码
│   ├── kernel/       # 陷阱处理、上下文切换
│   └── mm/           # 内存管理
├── kernel/           # 内核核心
│   ├── fork.c        # Fork 实现
│   ├── exit.c        # Exit/Wait 实现
│   ├── exec.c        # ELF 加载器
│   ├── sched_new.c   # O(1) 调度器
│   └── shell.c       # 交互式 Shell
├── fs/               # 文件系统
└── drivers/          # 设备驱动
```

---

## 文档

- **[BUGS.md](BUGS.md)** - 待修复问题清单
- **[HowToFitPosix.md](HowToFitPosix.md)** - POSIX 实现路线图
- **[CLAUDE.md](CLAUDE.md)** - 开发者指南
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - 系统架构

---

## 开发路线图

| 阶段 | 描述 | 状态 |
|------|------|------|
| Stage 1 | 基础架构、UART、Shell | ✅ 完成 |
| Stage 2 | 进程管理、调度器、Fork | ✅ 完成 |
| Stage 3 | 内存管理、COW、mmap | 计划中 |
| Stage 4 | 完整系统调用、信号 | 计划中 |
| Stage 5 | musl libc 移植 | 计划中 |
| Stage 6 | 高级特性 | 计划中 |

---

**版本**: 25.22.11
**状态**: 活跃开发中
**最后更新**: 2025-12-11
