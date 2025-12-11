# MinixRV64 Bug Tracking - 待修复问题

**创建日期**: 2025-12-11
**状态**: 活跃开发中

本文档记录 MinixRV64 实现中需要修复的 bug 和待完成的功能。

---

## 高优先级问题

### BUG-001: kmalloc 返回无效地址
**状态**: 🔴 未修复
**文件**: `arch/riscv64/mm/slab.c`
**描述**: kmalloc 分配器返回无效或重叠的内存地址，导致数据结构损坏。
**影响**:
- 无法动态分配 task_struct
- 无法动态分配 mm_struct
- 文件描述符分配可能失败
**当前 Workaround**: 使用静态数组 `task_cache[NR_TASKS]` 代替动态分配
**修复方案**:
1. 检查 slab allocator 初始化
2. 验证内存区域不重叠
3. 添加内存分配调试输出

---

### BUG-002: 调度器未完全集成
**状态**: 🟡 部分完成
**文件**: `kernel/sched_new.c`, `kernel/init_proc.c`
**描述**: O(1) 调度器框架已实现，但未完全集成到系统主循环。
**影响**:
- schedule() 可能不会切换到正确的进程
- 时间片计数未正确递减
- 抢占式调度未启用
**待完成**:
1. 在 timer 中断处理中调用 scheduler_tick()
2. 在 trap 返回路径检查 need_resched()
3. 启用抢占计数和调度点

---

### BUG-003: Fork 缺少 Copy-on-Write (COW)
**状态**: 🔴 未实现
**文件**: `kernel/fork.c`, `arch/riscv64/mm/pgtable.c`
**描述**: fork 实现只做了浅拷贝，没有实现 COW 机制。当前 `copy_mm()` 不复制页表。
**影响**:
- 子进程没有独立的地址空间
- 内存使用效率低
- 大进程 fork 非常慢
- 可能耗尽物理内存

**当前代码问题** (`kernel/fork.c:359-363`):
```c
/* TODO: Allocate new page table and implement COW */
/* For now, we don't copy page tables */
mm->pgd = NULL;    // ❌ 没有复制页表！
mm->mmap = NULL;   // ❌ 没有复制VMA！
```

**COW 实现检查清单**:
| 检查项 | 状态 | 说明 |
|--------|------|------|
| TLB一致性：修改页表后 flush_tlb_mm | 🔴 未实现 | 需要在设置只读后刷新 TLB |
| 原子性：pte_wrprotect/set_pte 关中断 | 🔴 未实现 | 防止时钟中断调度 |
| 引用计数：get_page 在 set_pte 之前 | 🔴 未实现 | 需要页面引用计数 |
| 错误路径：alloc_page 失败回退 | 🔴 未实现 | 需要回退已复制的 PTE |
| NULL检查：pte_offset 返回检查 pte_none | 🔴 未实现 | walk_pgtable 有检查但 COW 未使用 |
| 页大小统一：只处理 4KB 页 | ⚠️ 需注意 | 2MB/1GB 大页需直接复制，不能 COW |

**缺失的关键函数** (声明在 `mm_types.h` 但未实现):
- `copy_page_range()` - 复制页表范围并设置 COW
- `do_cow_fault()` - 处理 COW 页面错误
- `dup_mm()` - 完整复制 mm_struct

**待实现步骤**:
1. **页表复制** - `copy_page_range()` 遍历 VMA 并复制 PTE
2. **写保护设置** - 清除父子进程的 PTE_W 位
3. **页面引用计数** - `struct page` 添加 `_mapcount` 字段
4. **页面错误处理** - `do_cow_fault()` 在写时复制页面
5. **TLB 刷新** - 修改 PTE 后调用 `sfence.vma`

---

## 中优先级问题

### BUG-004: ELF 加载器需要 VFS 集成
**状态**: 🟡 部分完成
**文件**: `kernel/exec.c`
**描述**: ELF 验证和段加载框架存在，但无法从文件系统读取 ELF 文件。
**影响**:
- `execve()` 系统调用返回 -ENOEXEC
- 无法运行用户态程序
**待实现**:
```c
// do_execve() 需要:
// 1. vfs_open(filename, O_RDONLY)
// 2. vfs_read() 读取 ELF 头
// 3. 验证 ELF
// 4. vfs_read() 读取各段
// 5. 调用 load_elf_from_memory()
```

---

### BUG-005: 用户态返回路径未测试
**状态**: 🟡 已实现未测试
**文件**: `arch/riscv64/kernel/swtch.S`
**描述**: `ret_to_user` 汇编代码已编写，但未经实际用户态程序测试。
**影响**:
- 可能无法正确返回用户态
- 寄存器恢复可能有遗漏
- sstatus 设置可能不正确
**测试方案**:
1. 创建简单的用户态 ELF 程序
2. 通过 execve 加载
3. 验证用户态执行
4. 验证系统调用返回

---

### BUG-006: 僵尸进程回收不完整
**状态**: 🟡 部分完成
**文件**: `kernel/exit.c`, `kernel/init_proc.c`
**描述**: do_wait() 实现存在，但 init 进程的僵尸回收循环可能不工作。
**影响**:
- 孤儿进程可能不被正确收养
- 僵尸进程可能累积
**待验证**:
1. 测试父进程退出时子进程是否被 init 收养
2. 测试 wait() 是否正确回收僵尸
3. 测试 WNOHANG 选项

---

### BUG-007: idle_thread 被注释
**状态**: 🟡 暂时禁用
**文件**: `kernel/init_proc.c`
**描述**: idle_thread 函数被 `#if 0` 注释以避免未使用警告。
**影响**:
- CPU 空闲时没有专门的 idle 线程
- 可能影响电源管理
**修复方案**:
```c
// 在调度器完全集成后:
// 1. 移除 #if 0
// 2. 在 setup_idle_process() 中启动 idle 线程
// 3. idle 线程执行 wfi 指令
```

---

## 低优先级问题

### BUG-008: 进程数限制为 64
**状态**: 🔵 设计限制
**文件**: `kernel/fork.c`
**描述**: `task_cache[NR_TASKS]` 静态数组限制最大进程数为 64。
**影响**:
- 无法创建超过 64 个进程
- 适合嵌入式场景，但对完整系统不足
**未来改进**:
- 修复 kmalloc 后改用动态分配
- 或增加 NR_TASKS 常量值

---

### BUG-009: 文件描述符限制为 16
**状态**: 🔵 设计限制
**文件**: `include/minix/task.h`
**描述**: `MAX_OPEN_FILES = 16` 限制每进程打开文件数。
**影响**:
- 复杂程序可能耗尽文件描述符
**改进方案**:
- 增加 MAX_OPEN_FILES
- 或实现动态文件描述符表

---

### BUG-010: 信号处理未实现
**状态**: 🔵 未开始
**文件**: 需要新建
**描述**: 完整的信号处理机制未实现。
**影响**:
- kill 命令只能终止进程，无法发送其他信号
- 无法实现 SIGCHLD、SIGTERM 等标准行为
**Stage 4 任务**:
1. 定义信号结构
2. 实现 sigaction()
3. 实现信号传递机制
4. 在 trap 返回路径检查待处理信号

---

## 代码质量问题

### CLEANUP-001: 重复的内存操作函数
**文件**: 多个文件
**描述**: `memcpy_local`、`memset_local` 等函数在多个文件中重复定义。
**改进**: 统一使用 `lib/string.c` 中的实现。

### CLEANUP-002: 调试输出过多
**文件**: 多个文件
**描述**: 大量 `early_puts()` 调试输出影响启动速度。
**改进**: 添加调试级别控制，或使用条件编译。

### CLEANUP-003: 类型定义分散
**文件**: `types.h`, `task.h`, `sched.h`
**描述**: pid_t、uid_t 等类型在多处定义，需要 `#ifndef` 保护。
**改进**: 统一在一个头文件中定义所有基本类型。

---

## 测试清单

### 基础测试
- [x] 内核编译通过
- [x] QEMU 启动正常
- [x] Shell 交互正常
- [ ] schedule() 切换进程
- [ ] kernel_thread() 创建线程
- [ ] 内核线程执行
- [ ] fork() 系统调用
- [ ] exit() 系统调用
- [ ] wait() 系统调用

### 进阶测试
- [ ] 多进程并发运行
- [ ] 僵尸进程回收
- [ ] 孤儿进程收养
- [ ] ELF 加载执行
- [ ] 用户态程序运行
- [ ] COW 页面错误处理

---

## 修复进度

| Bug ID | 描述 | 优先级 | 状态 | 预计阶段 |
|--------|------|--------|------|----------|
| BUG-001 | kmalloc 问题 | 高 | 🔴 | Stage 3 |
| BUG-002 | 调度器集成 | 高 | 🟡 | Stage 2.5 |
| BUG-003 | COW Fork | 高 | 🔴 | Stage 3 |
| BUG-004 | ELF+VFS | 中 | 🟡 | Stage 3 |
| BUG-005 | 用户态返回 | 中 | 🟡 | Stage 4 |
| BUG-006 | 僵尸回收 | 中 | 🟡 | Stage 2.5 |
| BUG-007 | idle 线程 | 中 | 🟡 | Stage 2.5 |
| BUG-008 | 进程数限制 | 低 | 🔵 | Stage 3 |
| BUG-009 | FD 限制 | 低 | 🔵 | Stage 3 |
| BUG-010 | 信号处理 | 低 | 🔵 | Stage 4 |

**图例**:
- 🔴 未修复 / 阻塞
- 🟡 进行中 / 部分完成
- 🔵 计划中 / 设计限制
- 🟢 已完成

---

**最后更新**: 2025-12-11
