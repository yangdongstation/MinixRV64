# MinixRV64 Donz Build

## 项目定位

**MinixRV64 Donz Build** 是一个 **Unix 风格操作系统**，以 **Minix 微内核思想**为主导，同时采用 **Linux 系统调用 ABI** 实现用户态兼容。

| 设计层面 | 采用方案 | 说明 |
|---------|---------|------|
| 内核架构 | **Minix 微内核** | 最小化内核、服务隔离、消息传递 IPC |
| 系统调用 | **Linux ABI** | RISC-V Linux syscall 编号，兼容 musl |
| C 标准库 | **musl libc** | 静态链接，直接运行 Linux 用户态程序 |
| 可执行格式 | **ELF** | 标准 64 位 ELF |

## 目标
将 Minix 操作系统移植到 RISC-V 64位架构，特别针对 MilkV Duo CV1800B 开发板。

**最新更新**: 2025-12-11 - Stage 2 进程管理框架实现完成！

**快速开始**: 运行 `make qemu` 即可使用完整的交互式文件系统！参见 [QUICK_START.md](QUICK_START.md)

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
│   ├── boot/         # 启动代码 (start.S)
│   ├── kernel/       # 陷阱处理、上下文切换
│   └── mm/           # 内存管理
├── drivers/          # 设备驱动
├── fs/               # 文件系统 (VFS, ramfs, ext2, fat32)
├── include/          # 头文件
│   └── minix/        # 内核头文件
│       ├── task.h    # [新] 增强的task_struct
│       ├── sched.h   # [新] O(1)调度器定义
│       ├── mm_types.h# [新] 内存管理类型
│       ├── thread_info.h # [新] 线程信息
│       └── elf.h     # [新] ELF格式定义
├── kernel/           # 内核核心
│   ├── fork.c        # [新] Fork实现、PID分配
│   ├── exit.c        # [新] Exit/Wait实现
│   ├── exec.c        # [新] ELF加载器
│   ├── sched_new.c   # [新] O(1)优先级调度器
│   ├── init_proc.c   # [新] Idle/Init进程
│   ├── syscalls.c    # [更新] Linux ABI系统调用
│   └── shell.c       # 交互式Shell
├── lib/              # 库函数
└── tools/            # 开发工具
```

## 系统状态

| 组件 | 状态 | 说明 |
|------|------|------|
| 内核启动 | ✅ 工作 | 完美初始化 |
| MMU | ⚠️ 禁用 | 运行在物理地址模式 |
| **O(1)调度器** | ✅ **新增** | **优先级数组、active/expired队列** |
| **进程管理** | ✅ **新增** | **task_struct、fork、exit、wait框架** |
| **内核线程** | ✅ **新增** | **kernel_thread()创建内核线程** |
| **ELF加载器** | ✅ **新增** | **ELF验证、段加载框架** |
| UART输出 | ✅ 工作 | 显示正常 |
| UART输入 | ✅ 已修复 | 键盘输入完全可用 |
| 交互式Shell | ✅ 工作 | 16个命令可用 |
| VFS | ✅ 完整 | 文件系统完全可用 |
| ramfs | ✅ 完整 | 读写、目录创建正常 |

## 开发阶段

### Stage 1: 基础架构 ✅
- RISC-V启动、中断处理
- UART驱动
- 基本Shell

### Stage 2: 进程管理 ✅ (2025-12-11)
- [x] 增强的task_struct (~70字段)
- [x] 内核栈和thread_info (8KB栈)
- [x] PID位图分配器 (32768 PIDs)
- [x] O(1)优先级调度器
- [x] Fork框架 (copy_process, do_fork)
- [x] Exit/Wait框架 (do_exit, do_wait, release_task)
- [x] 内核线程创建 (kernel_thread)
- [x] ELF加载器框架
- [x] 上下文切换汇编 (ret_from_fork, ret_to_user)
- [x] Linux ABI系统调用更新

详见: [STAGE2_BUGS.md](STAGE2_BUGS.md) - Stage 2待修复问题

### Stage 3: 内存管理完善 (计划中)
- 修复kmalloc/slab allocator
- Copy-on-Write fork
- 完整的虚拟内存映射
- mmap/munmap实现

### Stage 4: 系统调用完善 (计划中)
- 完整的Linux syscall表
- 用户态/内核态切换
- 信号处理

### Stage 5: C库移植 (计划中)
- musl libc移植
- 静态链接支持
- 动态链接框架

### Stage 6: 高级特性 (计划中)
- EXT2/FAT32完整实现
- 网络协议栈
- 多核支持

## 功能特性

### ✓ 已实现（完全可用）
- **基础架构和引导** - RISC-V S模式运行
- **UART驱动** - 输入输出完全正常
- **交互式Shell** - 16个命令，完整的命令行解析
- **VFS虚拟文件系统** - 路径解析、挂载管理
- **ramfs内存文件系统** - 完整的文件读写和目录操作
- **进程管理框架** - task_struct、调度器、fork/exit框架
- **文件系统命令**：mkdir, ls, cat, touch, write, mount
- **其他命令**：help, clear, echo, pwd, ps, kill, reboot, uname

### ⚠ 部分实现（框架存在）
- 内存管理 (kmalloc有bug，使用静态数组workaround)
- 进程调度 (O(1)调度器框架，需要完整集成)
- Fork/Exec (框架存在，需要COW和完整VFS集成)
- ELF加载 (验证和段加载框架，需要VFS读取)

### ☐ 未实现
- 完整的虚拟内存
- 用户态程序运行
- 信号处理
- EXT2/FAT32文件系统
- GPIO/SD卡驱动
- 网络协议栈

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

**预期输出**：
```
✓ O(1) Scheduler
✓ Fork subsystem
Minix RV64 ready
✓ Shell
minix# help
Available commands:
  help    - Show this help
  ...
minix#
```

### 调试
```bash
make qemu-gdb    # 终端1: 启动QEMU等待GDB连接
make gdb         # 终端2: 启动GDB并连接
```

## 交叉编译工具链
- 目标: riscv64-unknown-elf
- GCC 13+ with RV64GC support

## 文档

### 核心文档
- **[STAGE2_BUGS.md](STAGE2_BUGS.md)** - Stage 2待修复问题清单
- **[HowToFitPosix.md](HowToFitPosix.md)** - 完整的POSIX实现路线图
- [CLAUDE.md](CLAUDE.md) - 开发者指南
- [QUICK_START.md](QUICK_START.md) - 快速开始指南

### 技术文档
- [ARCHITECTURE.md](ARCHITECTURE.md) - 系统架构概览
- [FILESYSTEM.md](FILESYSTEM.md) - 文件系统设计
- [UART_DRIVER.md](UART_DRIVER.md) - UART驱动文档

## Stage 2 新增文件

### 头文件
| 文件 | 行数 | 功能 |
|------|------|------|
| `include/minix/task.h` | ~410 | 增强的task_struct、trapframe、context |
| `include/minix/sched.h` | ~230 | O(1)调度器、链表、进程状态/标志 |
| `include/minix/mm_types.h` | ~170 | mm_struct、vm_area_struct、atomic/spinlock |
| `include/minix/thread_info.h` | ~135 | thread_info、内核栈布局 |
| `include/minix/elf.h` | ~200 | ELF64定义 |

### 实现文件
| 文件 | 行数 | 功能 |
|------|------|------|
| `kernel/fork.c` | ~900 | PID分配、task分配、copy_process、do_fork、kernel_thread |
| `kernel/sched_new.c` | ~540 | O(1)调度器、context_switch、schedule、wake_up |
| `kernel/exit.c` | ~410 | do_exit、release_task、do_wait、sys_kill |
| `kernel/exec.c` | ~420 | ELF验证、段加载、setup_user_stack |
| `kernel/init_proc.c` | ~160 | setup_idle_process、create_init_process |
| `arch/riscv64/kernel/swtch.S` | ~180 | swtch、ret_from_fork、ret_to_user |

## 技术细节

### 进程数据结构
```c
struct task_struct {
    /* 调度 */
    volatile long state;        // TASK_RUNNING, TASK_ZOMBIE等
    unsigned int flags;         // PF_KTHREAD, PF_EXITING等
    int prio, static_prio;      // 优先级
    unsigned long time_slice;   // 时间片

    /* 标识 */
    pid_t pid, tgid, ppid;      // 进程ID
    uid_t uid, euid;            // 用户ID

    /* 内存 */
    struct mm_struct *mm;       // 用户地址空间
    void *stack;                // 内核栈 (thread_info)

    /* 上下文 */
    struct trapframe *trapframe;// 陷阱帧
    struct context context;     // 内核上下文

    /* 关系 */
    struct task_struct *parent; // 父进程
    struct list_head children;  // 子进程列表

    /* 文件 */
    file_desc_t *ofile[16];     // 打开的文件
    // ... 更多字段
};
```

### 内存映射
```
0x80000000      内核代码段 (.text)
0x80100000      内核数据段 (.data, .rodata)
0x80200000      BSS段
0x80300000      内核堆

内核栈布局 (8KB per process):
┌─────────────────────────┐ ← stack + THREAD_SIZE
│    struct trapframe     │   (陷阱帧在栈顶)
├─────────────────────────┤
│                         │
│    内核栈空间 (向下增长) │
│          ↓              │
├─────────────────────────┤
│   struct thread_info    │ ← stack (栈底)
└─────────────────────────┘
```

### 当前限制
- **进程数**: 最大64个进程 (task_cache静态数组)
- **PID范围**: 1-32767
- **内核栈**: 8KB per process
- **文件描述符**: 每进程16个
- **内存分配**: kmalloc有bug，使用静态数组

## 已知问题

详见 [STAGE2_BUGS.md](STAGE2_BUGS.md)

主要问题：
1. kmalloc返回无效地址
2. 调度器未完全集成到主循环
3. Fork缺少COW实现
4. ELF加载器需要VFS集成

## 贡献

欢迎贡献！特别是以下方面：
- 修复kmalloc/slab allocator
- 完善进程管理集成
- 实现Copy-on-Write
- 添加更多系统调用

## 许可

本项目基于Minix操作系统，遵循BSD开源许可证。

---

**最后更新**: 2025-12-11
**项目状态**: 🟢 活跃开发中
**当前里程碑**: Stage 2 进程管理框架完成
**功能完整度**: 约40% (进程管理框架级别)
