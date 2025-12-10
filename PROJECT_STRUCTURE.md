# MinixRV64 项目结构

## 目录树

```
MinixRV64/
├── arch/                       # 架构相关代码
│   └── riscv64/               # RISC-V 64位架构
│       ├── boot/              # 启动代码
│       │   └── start.S        # 启动汇编
│       ├── include/           # 架构头文件
│       │   ├── asm/           # 汇编相关
│       │   │   ├── csr.h      # CSR寄存器定义
│       │   │   └── io.h       # I/O访问宏
│       │   └── board/         # 板级配置
│       │       ├── cv1800b.h  # MilkV Duo配置
│       │       └── qemu-virt.h # QEMU virt配置
│       ├── kernel/            # 架构相关内核
│       │   ├── early_console.S # 早期控制台
│       │   ├── early_print.c  # 早期打印
│       │   ├── htif.c         # Host-Target Interface
│       │   ├── main.c         # 内核入口
│       │   ├── swtch.S        # 上下文切换
│       │   ├── trap.c         # 中断处理
│       │   └── trap_asm.S     # 中断汇编
│       └── mm/                # 内存管理
│           ├── mmu.c          # MMU管理
│           ├── page_alloc.c   # 页分配器
│           ├── pgtable.c      # 页表管理
│           └── slab.c         # Slab分配器
│
├── drivers/                   # 设备驱动
│   ├── block/                 # 块设备驱动
│   │   └── blockdev.c         # 块设备接口
│   └── char/                  # 字符设备驱动
│       └── uart.c             # UART驱动 ★
│
├── fs/                        # 文件系统
│   ├── devfs.c               # 设备文件系统 ★
│   ├── ext2.c                # EXT2文件系统
│   ├── ext3.c                # EXT3文件系统
│   ├── ext4.c                # EXT4文件系统
│   ├── fat.c                 # FAT文件系统
│   ├── fat32.c               # FAT32文件系统
│   ├── ramfs.c               # RAM文件系统 ★
│   └── vfs.c                 # 虚拟文件系统 ★
│
├── include/                   # 全局头文件
│   ├── asm/                   # 汇编头文件
│   │   ├── csr.h             # CSR寄存器
│   │   └── io.h              # I/O访问
│   ├── board/                 # 板级头文件
│   │   ├── cv1800b.h         # CV1800B配置
│   │   └── qemu-virt.h       # QEMU配置
│   ├── minix/                 # Minix头文件
│   │   ├── blockdev.h        # 块设备接口
│   │   ├── blockdev_priv.h   # 块设备私有
│   │   ├── board.h           # 板级抽象
│   │   ├── config.h          # 配置
│   │   ├── print.h           # 打印函数
│   │   ├── process.h         # 进程管理
│   │   ├── shell.h           # Shell接口
│   │   ├── syscall.h         # 系统调用
│   │   ├── uart.h            # UART接口 ★
│   │   └── vfs.h             # VFS接口
│   ├── stdarg.h              # 标准参数
│   └── types.h               # 类型定义
│
├── kernel/                    # 内核核心
│   ├── board.c               # 板级初始化
│   ├── drivers.c             # 驱动管理
│   ├── proc.c                # 进程管理
│   ├── sched.c               # 调度器
│   ├── shell.c               # 内核Shell
│   └── syscalls.c            # 系统调用
│
├── lib/                       # 库函数
│   ├── printk.c              # 内核打印
│   └── string.c              # 字符串函数
│
├── scripts/                   # 构建脚本
│
├── docs/                      # 文档 (本目录)
│   ├── ARCHITECTURE.md       # 架构文档 ★
│   ├── FILESYSTEM.md         # 文件系统文档 ★
│   ├── UART_DRIVER.md        # UART驱动文档 ★
│   └── PROJECT_STRUCTURE.md  # 本文档 ★
│
├── Makefile                   # 主Makefile ★
├── build.sh                   # 构建脚本
├── run_test.sh               # 测试脚本
├── README.md                  # 项目说明
└── .gitignore                # Git忽略文件

★ = 最近更新/新增
```

## 核心模块说明

### 1. 启动流程
```
arch/riscv64/boot/start.S
  ↓
arch/riscv64/kernel/main.c:kernel_main()
  ↓
kernel/board.c:board_init()
  ↓
drivers/char/uart.c:console_init()
  ↓
fs/vfs.c:vfs_init()
  ↓
kernel/shell.c:shell_main()
```

### 2. 内存管理
```
arch/riscv64/mm/
├── page_alloc.c    # 物理页分配 (伙伴算法)
├── pgtable.c       # 页表管理 (SV39)
├── slab.c          # 内核对象分配
└── mmu.c           # MMU初始化
```

### 3. 文件系统栈
```
应用层
  ↓
fs/vfs.c (VFS层)
  ↓ ↓ ↓
devfs ramfs ext2
  ↓     ↓    ↓
设备  内存  块设备
```

### 4. 驱动框架
```
kernel/drivers.c (驱动管理)
  ↓
drivers/
├── char/uart.c     # 字符设备
└── block/          # 块设备
```

## 编译流程

### 1. 源文件 → 目标文件
```makefile
%.c → %.o   (GCC编译C源码)
%.S → %.o   (GCC编译汇编)
```

### 2. 链接
```
所有.o文件 → minix-rv64.elf (链接脚本: kernel.ld)
```

### 3. 生成二进制
```
minix-rv64.elf → minix-rv64.bin (objcopy)
```

## 配置系统

### 板级选择
```bash
# QEMU (默认)
make BOARD=qemu-virt

# MilkV Duo
make BOARD=milkv-duo
```

### 配置文件
- `include/minix/config.h` - 全局配置
- `include/board/*.h` - 板级配置
- `Makefile` - 编译配置

### 重要配置项
```c
// include/minix/config.h
#define BOARD_QEMU_VIRT     2
#define BOARD_MILKV_DUO     1

// 当前板级通过 -DBOARD=X 定义
#if BOARD == BOARD_QEMU_VIRT
    #define BOARD_UART_BASE 0x10000000
#elif BOARD == BOARD_MILKV_DUO
    #define BOARD_UART_BASE 0x04140000
#endif
```

## 文件命名规范

### 源文件
- `.c` - C源文件
- `.S` - 汇编源文件 (预处理)
- `.s` - 纯汇编源文件 (不预处理)
- `.h` - 头文件

### 目标文件
- `.o` - 目标文件
- `.elf` - ELF可执行文件
- `.bin` - 原始二进制文件

### 命名风格
- 文件名: `snake_case.c`
- 函数名: `snake_case()`
- 类型名: `snake_case_t`
- 宏定义: `UPPER_CASE`

## 依赖关系

### 核心依赖图
```
types.h (基础类型)
  ↓
config.h, board.h (配置)
  ↓
asm/*.h (硬件抽象)
  ↓
minix/*.h (内核接口)
  ↓
*.c (实现)
```

### 模块依赖
```
VFS
 ├─ devfs
 ├─ ramfs
 └─ ext2/fat32
     └─ blockdev

UART
 └─ board config

Memory Management
 ├─ page_alloc
 ├─ slab
 └─ pgtable
```

## 构建产物

### 主要输出
```
minix-rv64.elf    # ELF格式内核 (用于调试)
minix-rv64.bin    # 二进制内核 (用于部署)
*.o               # 目标文件
```

### 清理
```bash
make clean        # 删除所有构建产物
```

## 开发工作流

### 1. 添加新驱动
```bash
# 1. 创建源文件
touch drivers/char/newdev.c

# 2. 添加头文件
touch include/minix/newdev.h

# 3. 更新Makefile
# 在C_SRCS添加: $(DRIVER_DIR)/char/newdev.c

# 4. 实现驱动接口
# 5. 注册驱动
# 6. 编译测试
make clean && make
```

### 2. 添加新文件系统
```bash
# 1. 创建实现
touch fs/newfs.c

# 2. 实现fs_ops_t接口
# 3. 注册到VFS
# 4. 更新Makefile
# 在C_SRCS添加: $(FS_DIR)/newfs.c

# 5. 编译测试
make clean && make
```

### 3. 修改配置
```bash
# 1. 编辑配置文件
vim include/minix/config.h

# 2. 或板级配置
vim include/board/qemu-virt.h

# 3. 重新编译
make clean && make
```

## 测试

### QEMU测试
```bash
# 编译并运行
make qemu

# 调试模式
make qemu-debug

# GDB调试
make qemu-gdb
# 另一个终端
make gdb
```

### 真机测试 (MilkV Duo)
```bash
# 编译
make BOARD=milkv-duo

# 部署 (通过SD卡或USB)
# 具体步骤见 README_QEMU.md
```

## 代码统计

### 按模块
```
arch/riscv64/    ~2000 行
drivers/         ~500 行
fs/              ~2500 行
kernel/          ~1500 行
lib/             ~300 行
include/         ~1000 行
────────────────────────
总计:            ~7800 行
```

### 按语言
```
C:       ~6500 行
汇编:     ~800 行
头文件:   ~500 行
```

## 贡献指南

### 代码风格
- 遵循Linux内核代码风格
- 使用Tab缩进 (8空格)
- 函数名和变量名使用snake_case
- 宏定义使用UPPER_CASE

### 提交规范
```
[模块] 简短描述

详细说明改动内容和原因

Signed-off-by: Your Name <email@example.com>
```

### 文档更新
- 添加新功能时更新对应文档
- 保持 ARCHITECTURE.md 最新
- 重大改动更新 README.md

## 参考资源

### 内部文档
- [ARCHITECTURE.md](ARCHITECTURE.md) - 系统架构
- [FILESYSTEM.md](FILESYSTEM.md) - 文件系统设计
- [UART_DRIVER.md](UART_DRIVER.md) - UART驱动详解
- [README.md](README.md) - 快速开始

### 外部资源
- [RISC-V规范](https://riscv.org/specifications/)
- [16550 UART规范](https://www.ti.com/lit/ds/symlink/pc16550d.pdf)
- [EXT2文件系统](https://www.nongnu.org/ext2-doc/)
- [Minix3文档](https://wiki.minix3.org/)
