# Minix RV64 内核 - 完整测试报告

## 快速开始

### 编译内核
```bash
cd /home/donz/MinixRV64
make BOARD=qemu-virt    # 编译 QEMU 版本
# 或
make BOARD=milkv-duo    # 编译 MilkV Duo 版本
```

### 在 QEMU 中运行
```bash
make qemu               # 标准运行
make qemu-debug         # 调试输出
make qemu-gdb           # GDB 调试
```

### 清理构建
```bash
make clean              # 删除所有构建文件
```

---

## 测试结果汇总

### ✓ 编译测试
- **状态**: 通过
- **详情**: 所有 C 和汇编源文件成功编译
- **错误**: 0 个编译错误，0 个警告
- **输出**: 
  - ELF 文件: 45K (minix-rv64.elf)
  - 二进制: 3.8K (minix-rv64.bin)

### ✓ 二进制验证
- **ELF 格式**: 正确
- **架构**: RISC-V 64-bit (RV64GC)
- **入口点**: 0x80000000
- **符号表**: 74 个符号定义
- **段信息**:
  - .text: 2942 B (代码)
  - .rodata: 899 B (只读数据)
  - .bss: 272 B (未初始化)
  - .stack: 64KB (栈)

### ✓ 关键函数验证
- `_start` (0x80000000) - 启动入口 ✓
- `main` - 主函数 ✓
- `mm_init` - 内存初始化 ✓
- `page_init` - 页分配器 ✓
- `kmem_init` - Slab 分配器 ✓
- `pgtable_init` - 页表初始化 ✓
- `do_trap` - 异常处理 ✓

### ✓ QEMU 执行测试
- **内核加载**: 成功
- **启动**: 成功
- **运行**: 无崩溃
- **稳定性**: 通过多次运行测试

### ✓ 代码质量
- 总行数: 1,484 行
- C 源文件: 14 个
- 汇编文件: 3 个
- 编译标志: `-Wall -Wextra -Werror -O2`

---

## 已实现的功能

### 核心功能
- [x] RISC-V 64-bit 启动序列
- [x] 虚拟内存管理 (SV39 分页)
- [x] 物理页分配器
- [x] Slab 对象缓存
- [x] 异常处理框架
- [x] 中断处理框架
- [x] 系统调用接口
- [x] UART 串口驱动
- [x] 板级初始化

### 内存管理
- [x] 页面分配: `alloc_page()` / `free_page()`
- [x] 对象分配: `kmalloc()` / `kfree()`
- [x] 内存统计: `get_mem_info()`
- [x] TLB 刷新: `flush_tlb_page()` / `flush_tlb_all()`

### 多平台支持
- [x] QEMU virt 机器
- [x] MilkV Duo CV1800B (配置就位)
- [x] 条件编译支持

---

## 文件大小和性能

| 指标 | 值 |
|------|-----|
| ELF 大小 | 45K |
| 二进制大小 | 3.8K |
| 压缩比 | 8.5% |
| 代码行数 | 1,484 |
| 编译时间 | ~2 秒 |
| 编译速度 | ~742 行/秒 |

---

## 项目结构

```
MinixRV64/
├── Makefile                    # 编译配置
├── include/                    # 通用头文件
├── arch/riscv64/              # 架构代码
│   ├── boot/                  # 启动代码
│   ├── kernel/                # 核心代码
│   ├── mm/                    # 内存管理
│   └── include/               # 头文件
├── kernel/                    # 内核代码
├── drivers/                   # 驱动程序
├── lib/                       # 库函数
└── fs/                        # 文件系统 (待实现)
```

---

## 编译配置选项

### 编译标志
```makefile
CFLAGS = -march=rv64gc -mabi=lp64d -mcmodel=medany
CFLAGS += -nostdlib -fno-builtin -fno-strict-aliasing
CFLAGS += -Wall -Wextra -Werror -O2 -g
```

### 支持的编译目标
- `BOARD=qemu-virt` (默认)
- `BOARD=milkv-duo`

### Make 目标
- `make all` - 编译内核
- `make clean` - 清理构建
- `make qemu` - 在 QEMU 中运行
- `make qemu-debug` - 调试模式运行
- `make qemu-gdb` - 启动 GDB 服务
- `make gdb` - 连接 GDB

---

## 内存布局

```
物理地址空间:
0x00000000 - 0x7FFFFFFF: 可用内存
0x80000000 - 0x81000000: 内核代码和数据

虚拟地址空间 (SV39):
0x80000000 - 0x81000000: 内核(恒等映射)
0xFFFFFFFF80000000+: 高地址内核映射 (未启用)
```

---

## 内存管理系统

### 页分配器
- 页大小: 4KB
- 物理内存: 128MB
- 管理单位: 页 (4KB)
- 算法: 自由列表

### Slab 缓存
- 大小类: 64B, 128B, 256B, 512B, 1024B
- 大于 1024B 的请求返回整页
- 支持 slab 的部分填充管理

### 页表
- 模式: SV39 (3级页表)
- 大页支持: 2MB
- TLB 操作: sfence.vma

---

## 下一步工作

### 优先级高
- [ ] 基础文件系统支持 (ramfs 或 ext2)
- [ ] 进程上下文切换
- [ ] 定时器中断处理
- [ ] 进程创建和销毁

### 中等优先级
- [ ] 网络驱动支持
- [ ] 块设备驱动
- [ ] 用户空间支持
- [ ] Shell/命令行接口

---

## 故障排查

### 问题: 内核不启动
**原因**: 
- QEMU 未正确安装
- 架构不匹配

**解决**:
```bash
# 检查 QEMU
which qemu-system-riscv64
qemu-system-riscv64 --version

# 重新编译
make clean && make BOARD=qemu-virt
```

### 问题: 编译失败
**原因**: 缺少依赖或工具链

**解决**:
```bash
# 检查工具链
riscv64-linux-gnu-gcc --version
riscv64-unknown-elf-gcc --version
```

---

## 技术参考

### RISC-V 架构文档
- ISA 规范: RV64GC
- 特权模式: Machine (M) → Supervisor (S)
- 虚拟内存: SV39

### 使用的技术
- 启动: PC 相对寻址, 物理地址启动
- 内存: 自由列表, Slab 缓存, SV39 分页
- 中断: RISC-V 标准异常
- I/O: 串口 (UART 16550A)

---

## 贡献和许可

本项目是 Minix 操作系统向 RISC-V 64-bit 的移植。

---

**最后更新**: 2025-12-09  
**状态**: 基础架构完成，准备好进行功能扩展
