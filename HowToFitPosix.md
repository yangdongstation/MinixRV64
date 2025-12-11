● MinixRV64 Donz Build - POSIX兼容系统技术路线

  📋 总览

  项目定位：
  - **Unix 风格操作系统** - 学习和致敬 Minix 的设计理念
  - **微内核架构** - 以 Minix 微内核思想为主导
  - **Linux 兼容层** - 采用 Linux 系统调用 ABI，便于移植用户态程序
  - **教育与实践** - 适合学习操作系统原理和 RISC-V 架构

  设计哲学：
  ┌─────────────────────────────────────────────────────────────┐
  │  Minix 微内核精神          │  Linux 实用特性              │
  │  ─────────────────────     │  ─────────────────────       │
  │  • 最小化内核               │  • Linux syscall ABI        │
  │  • 服务隔离                 │  • musl libc 兼容           │
  │  • 消息传递 IPC             │  • ELF 可执行文件格式       │
  │  • 可靠性优先               │  • /proc, /sys 文件系统     │
  │  • 简洁清晰的代码           │  • POSIX 标准接口           │
  └─────────────────────────────────────────────────────────────┘

  为什么这样设计？
  1. **微内核优势** - 更好的模块化、可维护性、可靠性
  2. **Linux ABI** - 直接运行 musl 编译的程序，无需移植
  3. **学习价值** - 同时理解两种设计思想

  目标：将MinixRV64从教育演示级别提升到基本POSIX兼容的操作系统

  预计时间：12-24个月（全职开发）

  难度级别：⭐⭐⭐⭐⭐ 非常困难

  ---
  🗺️ 技术路线图（6大阶段）

  阶段1: 内存管理完善 ✅ 已完成 (2024-12)
    ↓
  阶段2: 进程管理实现 (3-4个月) ← 当前阶段
    ↓
  阶段3: 系统调用层 (2-3个月)
    ↓
  阶段4: C标准库移植 (1-2个月)
    ↓
  阶段5: 文件系统完善 (2-3个月)
    ↓
  阶段6: 高级特性与测试 (2-4个月)

  ---
  ✅ 阶段1：内存管理完善（已完成）

  完成日期：2024年12月

  已解决的问题

  - ✅ kmalloc返回有效地址（slab allocator已修复）
  - ✅ MMU已启用（SV39模式）
  - ✅ 虚拟内存管理（identity mapping + vmalloc）
  - ✅ 基本内存保护（页表权限位）

  ═══════════════════════════════════════════════════════════════
  任务1.1：修复kmalloc/slab allocator ✅ 已完成
  ═══════════════════════════════════════════════════════════════

  实现文件：arch/riscv64/mm/slab.c（完全重写）

  已实现功能：
  1. 正确的slab cache管理
     - 独立的cache结构分配
     - 支持partial/full/empty三个slab链表
     - 正确的free list管理（嵌入式链表）

  2. 支持的对象大小：32B, 64B, 128B, 256B, 512B, 1KB, 2KB
     - 注：4KB对象直接使用page allocator

  3. 调试工具已实现：
     void kmalloc_stats(void);   // 打印分配统计
     void kmalloc_dump(void);    // 详细dump
     int kmalloc_verify(void);   // 完整性检查

  关键数据结构：
  struct slab {
      struct slab *next, *prev;
      struct slab_cache *cache;
      void *free_list;
      unsigned int inuse, total;
  };

  struct slab_cache {
      const char *name;
      unsigned long obj_size, align;
      unsigned int objs_per_slab, slab_count;
      struct slab *slabs_partial, *slabs_full, *slabs_empty;
      unsigned long total_allocs, total_frees;
  };

  验收结果：
  [SLAB] Initializing slab allocator...
  [SLAB] Slab allocator initialized
  - kmalloc能正确分配32B到2KB的内存 ✅
  - 内存能正确释放和重用 ✅
  - 无内存泄漏 ✅

  ═══════════════════════════════════════════════════════════════
  任务1.2：实现buddy分配器 ✅ 已完成
  ═══════════════════════════════════════════════════════════════

  实现文件：arch/riscv64/mm/page_alloc.c（完全重写）

  已实现功能：
  1. 完整的buddy system算法
     - 支持order 0-11（4KB到8MB连续分配）
     - 页面分裂和合并（coalescing）
     - 高效的free list管理

  2. 核心API：
     unsigned long alloc_pages(int order);  // 分配2^order页
     void free_pages(unsigned long addr, int order);
     unsigned long alloc_page(void);        // 便捷函数
     void free_page(unsigned long addr);

  3. 调试工具：
     void buddy_stats(void);    // 打印各order的free block数量
     void get_mem_info(unsigned long *total, unsigned long *free);

  关键数据结构：
  struct page {
      unsigned long flags;
      int order;
      struct page *next, *prev;
      unsigned long ref_count;
  };

  struct free_area {
      struct page *free_list;
      unsigned long nr_free;
  };

  验收结果：
  [BUDDY] Memory range: 0x80400000 - 0x88000000
  [BUDDY] Total managed pages: 0x7C00 (31744页 = 124MB)
  [BUDDY] Free pages: 0x7ACA
  [BUDDY] Reserved pages: 0x136 (mem_map数组)

  ═══════════════════════════════════════════════════════════════
  任务1.3：启用MMU和虚拟内存 ✅ 已完成
  ═══════════════════════════════════════════════════════════════

  实现文件：arch/riscv64/mm/pgtable.c（完全重写）

  已实现功能：
  1. 完整的SV39三级页表支持
     - PGD (512 entries) → PMD (512 entries) → PTE (512 entries)
     - 39位虚拟地址空间

  2. 多种页面大小：
     int map_page_4k(pgd_t *pgd, va, pa, flags);   // 4KB页
     int map_page_2m(pgd_t *pgd, va, pa, flags);   // 2MB大页
     int map_page_1g(pgd_t *pgd, va, pa, flags);   // 1GB巨页

  3. 区域映射：
     int map_region(pgd_t *pgd, va_start, pa_start, size, flags);
     int map_region_large(pgd_t *pgd, ...);  // 自动选择最大页

  4. TLB管理：
     void flush_tlb_all(void);
     void flush_tlb_page(unsigned long addr);
     void flush_tlb_mm(unsigned long asid);
     void flush_tlb_range(unsigned long start, unsigned long size);

  5. 调试工具：
     void dump_pte(unsigned long va);
     unsigned long lookup_pa(unsigned long va);

  当前内存布局（identity mapping）：
  0x00000000 - 0x3FFFFFFF : MMIO区域 (1GB gigapage)
  0x80000000 - 0xBFFFFFFF : 内核区域 (1GB gigapage)

  验收结果：
  [MMU] Root PGD: 0x8000A000
  [MMU] SATP value: 0x800000000008000A
  [MMU] SATP verified OK
  [MMU] MMU enabled successfully
  ✓ MMU enabled with SV39 paging

  ═══════════════════════════════════════════════════════════════
  任务1.4：实现vmalloc ✅ 已完成
  ═══════════════════════════════════════════════════════════════

  实现文件：arch/riscv64/mm/vmalloc.c（新建）

  已实现功能：
  1. 虚拟连续内存分配：
     void *vmalloc(unsigned long size);
     void *vzalloc(unsigned long size);  // 清零版本
     void vfree(void *addr);

  2. 页面数组映射：
     void *vmap(unsigned long *pages, unsigned long nr_pages, flags);
     void vunmap(void *addr);

  3. I/O内存映射：
     void *ioremap(unsigned long phys_addr, unsigned long size);
     void iounmap(void *addr);

  4. VM区域管理：
     struct vm_struct {
         void *addr;
         unsigned long size, flags, nr_pages;
         unsigned long *pages;
         struct vm_struct *next;
         int in_use;
     };

  vmalloc区域：0x84000000 - 0x88000000 (64MB)

  验收结果：
  [VMALLOC] Range: 0x84000000 - 0x88000000
  [VMALLOC] Initialized

  ═══════════════════════════════════════════════════════════════
  任务1.5：添加页面故障处理 ✅ 已完成
  ═══════════════════════════════════════════════════════════════

  实现文件：arch/riscv64/kernel/trap.c（重写）

  已实现功能：
  1. 详细的页面故障诊断：
     - 区分指令/加载/存储故障
     - 区分内核态/用户态故障
     - 打印完整的trap frame信息
     - 检测NULL指针访问

  2. 故障类型处理：
     void do_page_fault(struct trap_frame *tf, int fault_type);
     - FAULT_INST_FETCH: 指令页故障
     - FAULT_LOAD: 加载页故障
     - FAULT_STORE: 存储页故障

  3. 调试输出示例：
     ========== Load Page Fault ==========
       SEPC (PC):    0x80001234
       STVAL (Addr): 0x00000000
       SCAUSE:       0x000000000000000D
       Mode: Supervisor
       >>> NULL pointer dereference! <<<

  4. 预留框架（待实现）：
     - demand paging
     - copy-on-write (COW)
     - stack growth

  验收结果：
  [TRAP] Trap vector: 0x800000E8
  [TRAP] SIE: 0x222
  [TRAP] Trap handling initialized
  - 内核态页故障正确诊断并panic ✅
  - 用户态页故障框架就绪 ✅

  ═══════════════════════════════════════════════════════════════
  阶段1验收总结
  ═══════════════════════════════════════════════════════════════

  验收标准：
  - ✅ kmalloc/kfree正常工作
  - ✅ MMU启用并正常运行
  - ✅ 内核在虚拟地址空间运行（identity mapping）
  - ✅ 页面故障能正确处理（诊断并报告）

  新增/修改文件：
  - arch/riscv64/mm/slab.c      (重写)
  - arch/riscv64/mm/page_alloc.c (重写)
  - arch/riscv64/mm/pgtable.c   (重写)
  - arch/riscv64/mm/mmu.c       (更新)
  - arch/riscv64/mm/vmalloc.c   (新建)
  - arch/riscv64/kernel/trap.c  (重写)
  - include/minix/mm.h          (新建)

  启动日志摘要：
  === Memory Management Initialization ===
  [BUDDY] Buddy allocator initialized
  [SLAB] Slab allocator initialized
  [MMU] MMU enabled successfully
  [VMALLOC] Initialized
  === Memory Management Ready ===

  遗留问题：
  1. 4KB slab cache创建失败（预期行为，使用page allocator）
  2. 尚未实现高端内核虚拟地址（使用identity mapping）
  3. demand paging和COW待阶段2实现

  ---
  阶段2：进程管理实现（3-4个月）← 当前阶段

  ═══════════════════════════════════════════════════════════════
  概述
  ═══════════════════════════════════════════════════════════════

  当前问题：
  - ❌ 没有真正的进程
  - ❌ 没有进程调度
  - ❌ 没有上下文切换
  - ❌ 没有用户态/内核态分离

  目标：
  - ✅ 实现完整的进程抽象
  - ✅ 支持 fork/exec/wait/exit
  - ✅ 实现抢占式调度
  - ✅ 用户态程序可以运行

  进程状态机：

  ┌─────────┐  fork()   ┌─────────┐
  │  NEW    │─────────→│  READY  │←────────────────┐
  └─────────┘          └────┬────┘                 │
                            │                      │
                       调度 │                      │ I/O完成
                            ↓                      │ 时间片到
                       ┌─────────┐                 │
                       │ RUNNING │─────────────────┤
                       └────┬────┘                 │
                            │                      │
              exit()        │        I/O等待       │
                ↓           │           ↓          │
           ┌─────────┐      │      ┌─────────┐    │
           │ ZOMBIE  │      │      │ BLOCKED │────┘
           └─────────┘      │      └─────────┘
                            │
              wait()        │
                ↓           │
           ┌─────────┐      │
           │  DEAD   │←─────┘
           └─────────┘

  ═══════════════════════════════════════════════════════════════
  任务2.1：进程数据结构设计（1周）
  ═══════════════════════════════════════════════════════════════

  /* include/minix/sched.h */

  // 进程状态
  #define TASK_RUNNING        0   // 可运行（就绪或运行中）
  #define TASK_INTERRUPTIBLE  1   // 可中断睡眠
  #define TASK_UNINTERRUPTIBLE 2  // 不可中断睡眠
  #define TASK_ZOMBIE         4   // 僵尸进程
  #define TASK_STOPPED        8   // 已停止

  // 进程标志
  #define PF_KTHREAD          0x00000001  // 内核线程
  #define PF_EXITING          0x00000004  // 正在退出
  #define PF_FORKNOEXEC       0x00000040  // fork后未exec

  // 调度策略
  #define SCHED_NORMAL        0   // 普通时间片轮转
  #define SCHED_FIFO          1   // 实时先进先出
  #define SCHED_RR            2   // 实时轮转

  // 进程优先级
  #define MAX_PRIO            140
  #define DEFAULT_PRIO        120
  #define MAX_USER_PRIO       100
  #define MAX_RT_PRIO         100

  // 时间片（tick数）
  #define DEF_TIMESLICE       10  // 默认10个tick
  #define MIN_TIMESLICE       1
  #define MAX_TIMESLICE       100

  地址空间结构：

  /* include/minix/mm_types.h */

  struct mm_struct {
      pgd_t *pgd;                     // 页全局目录（页表根）

      // 代码段
      unsigned long start_code;       // 代码段起始
      unsigned long end_code;         // 代码段结束

      // 数据段
      unsigned long start_data;       // 数据段起始
      unsigned long end_data;         // 数据段结束

      // 堆
      unsigned long start_brk;        // 堆起始
      unsigned long brk;              // 堆当前位置（通过brk系统调用调整）

      // 栈
      unsigned long start_stack;      // 栈起始（高地址）
      unsigned long arg_start;        // 参数区起始
      unsigned long arg_end;          // 参数区结束
      unsigned long env_start;        // 环境变量区起始
      unsigned long env_end;          // 环境变量区结束

      // VMA 管理
      struct vm_area_struct *mmap;    // VMA链表头
      struct rb_root mm_rb;           // VMA红黑树（加速查找）
      int map_count;                  // VMA数量
      unsigned long total_vm;         // 总虚拟内存大小（页数）
      unsigned long locked_vm;        // 锁定的内存
      unsigned long data_vm;          // 数据段大小
      unsigned long exec_vm;          // 可执行段大小
      unsigned long stack_vm;         // 栈大小

      // 引用计数
      atomic_t mm_users;              // 使用此mm的进程数
      atomic_t mm_count;              // 引用计数

      spinlock_t page_table_lock;     // 保护页表
      struct rw_semaphore mmap_sem;   // 保护mmap操作

      // 用于exec
      unsigned long saved_auxv[AT_VECTOR_SIZE];  // 保存的auxv
  };

  VMA（虚拟内存区域）结构：

  struct vm_area_struct {
      unsigned long vm_start;         // 区域起始地址（包含）
      unsigned long vm_end;           // 区域结束地址（不包含）
      struct vm_area_struct *vm_next; // 链表下一个
      struct vm_area_struct *vm_prev; // 链表上一个
      struct rb_node vm_rb;           // 红黑树节点

      unsigned long vm_flags;         // 权限和属性标志
      // VM_READ, VM_WRITE, VM_EXEC, VM_SHARED, VM_GROWSDOWN（栈）

      pgprot_t vm_page_prot;          // 页保护位

      // 文件映射
      struct file *vm_file;           // 映射的文件（匿名映射为NULL）
      unsigned long vm_pgoff;         // 文件内偏移（页单位）

      // 操作函数
      const struct vm_operations_struct *vm_ops;

      // 私有数据
      void *vm_private_data;
  };

  // VMA标志
  #define VM_READ         0x00000001
  #define VM_WRITE        0x00000002
  #define VM_EXEC         0x00000004
  #define VM_SHARED       0x00000008
  #define VM_MAYREAD      0x00000010
  #define VM_MAYWRITE     0x00000020
  #define VM_MAYEXEC      0x00000040
  #define VM_GROWSDOWN    0x00000100  // 栈向下增长
  #define VM_DENYWRITE    0x00000800
  #define VM_LOCKED       0x00002000
  #define VM_STACK        0x00000100

  进程控制块（PCB）：

  struct task_struct {
      // ========== 调度相关 ==========
      volatile long state;            // 进程状态
      unsigned int flags;             // 进程标志
      int on_rq;                      // 是否在运行队列

      int prio;                       // 动态优先级
      int static_prio;                // 静态优先级
      int normal_prio;                // 普通优先级
      unsigned int rt_priority;       // 实时优先级
      unsigned int policy;            // 调度策略

      struct sched_entity se;         // 调度实体
      unsigned long time_slice;       // 剩余时间片
      unsigned long utime, stime;     // 用户态/内核态CPU时间
      unsigned long nvcsw, nivcsw;    // 自愿/非自愿上下文切换次数

      // ========== 进程标识 ==========
      pid_t pid;                      // 进程ID
      pid_t tgid;                     // 线程组ID（主线程的pid）
      pid_t ppid;                     // 父进程PID

      // ========== 内存管理 ==========
      struct mm_struct *mm;           // 用户空间内存描述符
      struct mm_struct *active_mm;    // 内核线程借用的mm

      // ========== 文件系统 ==========
      struct fs_struct *fs;           // 文件系统信息（cwd, root）
      struct files_struct *files;     // 打开文件表

      // ========== 信号 ==========
      struct signal_struct *signal;   // 共享信号处理
      struct sighand_struct *sighand; // 信号处理函数
      sigset_t blocked;               // 阻塞的信号
      sigset_t real_blocked;
      struct sigpending pending;      // 待处理信号

      // ========== 寄存器上下文 ==========
      struct thread_struct thread;    // CPU相关上下文
      unsigned long kernel_sp;        // 内核栈指针
      unsigned long user_sp;          // 用户栈指针

      // ========== 进程关系 ==========
      struct task_struct *parent;     // 父进程
      struct task_struct *real_parent;// 真正的父进程
      struct list_head children;      // 子进程链表
      struct list_head sibling;       // 兄弟进程链表

      // ========== 进程组和会话 ==========
      pid_t pgrp;                     // 进程组ID
      pid_t session;                  // 会话ID
      struct task_struct *group_leader;// 线程组leader

      // ========== 等待 ==========
      wait_queue_head_t wait_chldexit;// 等待子进程退出
      int exit_code;                  // 退出码
      int exit_signal;                // 退出时发送给父进程的信号

      // ========== 时间 ==========
      u64 start_time;                 // 启动时间
      u64 real_start_time;            // 单调启动时间

      // ========== 命令行 ==========
      char comm[16];                  // 可执行文件名

      // ========== 链表 ==========
      struct list_head tasks;         // 所有进程链表
      struct list_head run_list;      // 运行队列链表
  };

  // 内核线程信息（架构相关）
  struct thread_struct {
      // RISC-V 架构
      unsigned long ra;               // 返回地址
      unsigned long sp;               // 栈指针
      unsigned long s[12];            // s0-s11 callee-saved寄存器
      unsigned long sepc;             // 异常PC
      unsigned long sstatus;          // 状态寄存器
  };

  // 调度实体（用于CFS调度器）
  struct sched_entity {
      struct rb_node run_node;        // 红黑树节点
      unsigned int on_rq;             // 是否在队列
      u64 vruntime;                   // 虚拟运行时间
      u64 sum_exec_runtime;           // 总执行时间
      u64 prev_sum_exec_runtime;
  };

  ═══════════════════════════════════════════════════════════════
  任务2.2：内核栈和进程分配（1周）
  ═══════════════════════════════════════════════════════════════

  内核栈布局：

  每个进程有独立的内核栈（2页 = 8KB）：

  高地址
  ┌─────────────────────────────────┐ ← 栈顶 + THREAD_SIZE
  │         struct pt_regs          │   (trap时保存的寄存器)
  ├─────────────────────────────────┤
  │                                 │
  │         内核栈空间              │   (函数调用栈)
  │              ↓                  │
  │                                 │
  ├─────────────────────────────────┤
  │      struct thread_info         │   (线程信息)
  └─────────────────────────────────┘ ← 栈底
  低地址

  /* include/minix/thread_info.h */

  #define THREAD_SIZE     (2 * PAGE_SIZE)  // 8KB
  #define THREAD_MASK     (~(THREAD_SIZE - 1))

  struct thread_info {
      struct task_struct *task;       // 指向task_struct
      unsigned long flags;            // 线程标志
      int preempt_count;              // 抢占计数
      int cpu;                        // 当前CPU
      mm_segment_t addr_limit;        // 地址空间限制
  };

  // 线程标志
  #define TIF_SIGPENDING      0       // 有待处理信号
  #define TIF_NEED_RESCHED    1       // 需要重新调度
  #define TIF_SYSCALL_TRACE   2       // 系统调用跟踪

  // 获取当前thread_info（通过栈指针）
  static inline struct thread_info *current_thread_info(void)
  {
      unsigned long sp;
      asm volatile ("mv %0, sp" : "=r"(sp));
      return (struct thread_info *)(sp & THREAD_MASK);
  }

  // 获取当前进程
  #define current (current_thread_info()->task)

  进程分配实现：

  /* kernel/fork.c */

  // 内核栈 + thread_info 分配
  static struct thread_info *alloc_thread_info(void)
  {
      // 分配2页对齐的内存
      struct page *page = alloc_pages(1);  // 2^1 = 2页
      if (!page) return NULL;

      struct thread_info *ti = page_address(page);
      memset(ti, 0, sizeof(*ti));
      ti->preempt_count = 0;

      return ti;
  }

  // task_struct 分配（使用slab）
  static struct kmem_cache *task_struct_cachep;

  void __init fork_init(void)
  {
      task_struct_cachep = kmem_cache_create("task_struct",
                                              sizeof(struct task_struct),
                                              ARCH_MIN_TASKALIGN,
                                              SLAB_PANIC, NULL);
  }

  static struct task_struct *alloc_task_struct(void)
  {
      return kmem_cache_alloc(task_struct_cachep, GFP_KERNEL);
  }

  static void free_task_struct(struct task_struct *tsk)
  {
      kmem_cache_free(task_struct_cachep, tsk);
  }

  // PID分配（简化版：使用位图）
  #define PID_MAX     32768
  static DECLARE_BITMAP(pid_bitmap, PID_MAX);
  static spinlock_t pid_lock;
  static pid_t last_pid = 0;

  pid_t alloc_pid(void)
  {
      pid_t pid;

      spin_lock(&pid_lock);
      do {
          last_pid++;
          if (last_pid >= PID_MAX)
              last_pid = 1;
      } while (test_bit(last_pid, pid_bitmap));

      set_bit(last_pid, pid_bitmap);
      pid = last_pid;
      spin_unlock(&pid_lock);

      return pid;
  }

  void free_pid(pid_t pid)
  {
      spin_lock(&pid_lock);
      clear_bit(pid, pid_bitmap);
      spin_unlock(&pid_lock);
  }

  ═══════════════════════════════════════════════════════════════
  任务2.3：fork 实现 - Copy-On-Write（2周）
  ═══════════════════════════════════════════════════════════════

  fork 是 Unix 进程模型的核心。为了效率，使用 Copy-On-Write（COW）：

  fork() 执行流程：

  父进程                              子进程
     │                                  │
     │  fork() ──────────────────────→ 创建
     │                                  │
     │  返回 child_pid                  │  返回 0
     │                                  │
     ↓                                  ↓

  COW 原理：

  fork时：
  ┌──────────────────┐      ┌──────────────────┐
  │   父进程页表      │      │   子进程页表      │
  │  VA → PA (R/O)   │      │  VA → PA (R/O)   │ ← 共享同一物理页
  └──────────────────┘      └──────────────────┘
            │                        │
            └────────────┬───────────┘
                         ↓
               ┌──────────────────┐
               │    物理页        │
               │  refcount = 2    │
               └──────────────────┘

  写时：
  ┌──────────────────┐      ┌──────────────────┐
  │   父进程页表      │      │   子进程页表      │
  │  VA → PA1 (R/W)  │      │  VA → PA2 (R/W)  │
  └──────────────────┘      └──────────────────┘
            │                        │
            ↓                        ↓
  ┌──────────────────┐      ┌──────────────────┐
  │   物理页 PA1     │      │   物理页 PA2     │ ← 写时复制
  │  refcount = 1    │      │  refcount = 1    │
  └──────────────────┘      └──────────────────┘

  实现代码：

  /* kernel/fork.c */

  // 复制进程
  struct task_struct *copy_process(unsigned long clone_flags,
                                    unsigned long stack_start,
                                    struct pt_regs *regs)
  {
      struct task_struct *p;
      struct thread_info *ti;
      int retval;

      // 1. 分配task_struct
      p = alloc_task_struct();
      if (!p)
          return ERR_PTR(-ENOMEM);

      // 2. 分配内核栈和thread_info
      ti = alloc_thread_info();
      if (!ti) {
          free_task_struct(p);
          return ERR_PTR(-ENOMEM);
      }

      // 3. 复制父进程基本信息
      *p = *current;
      ti->task = p;
      p->stack = ti;

      // 4. 分配新PID
      p->pid = alloc_pid();
      p->tgid = (clone_flags & CLONE_THREAD) ? current->tgid : p->pid;
      p->ppid = current->pid;

      // 5. 初始化进程状态
      p->state = TASK_RUNNING;
      p->flags &= ~PF_FORKNOEXEC;
      p->flags |= PF_FORKNOEXEC;

      // 6. 复制内存空间
      retval = copy_mm(clone_flags, p);
      if (retval)
          goto bad_fork_cleanup;

      // 7. 复制文件描述符表
      retval = copy_files(clone_flags, p);
      if (retval)
          goto bad_fork_cleanup_mm;

      // 8. 复制文件系统信息
      retval = copy_fs(clone_flags, p);
      if (retval)
          goto bad_fork_cleanup_files;

      // 9. 复制信号处理
      retval = copy_sighand(clone_flags, p);
      if (retval)
          goto bad_fork_cleanup_fs;

      // 10. 复制CPU上下文
      retval = copy_thread(clone_flags, stack_start, regs, p);
      if (retval)
          goto bad_fork_cleanup_sighand;

      // 11. 初始化调度相关
      p->time_slice = DEF_TIMESLICE;
      p->prio = current->prio;
      INIT_LIST_HEAD(&p->children);
      INIT_LIST_HEAD(&p->sibling);

      // 12. 建立父子关系
      p->parent = current;
      p->real_parent = current;
      list_add_tail(&p->sibling, &current->children);

      // 13. 添加到全局进程链表
      list_add_tail(&p->tasks, &init_task.tasks);

      return p;

  bad_fork_cleanup_sighand:
      exit_sighand(p);
  bad_fork_cleanup_fs:
      exit_fs(p);
  bad_fork_cleanup_files:
      exit_files(p);
  bad_fork_cleanup_mm:
      exit_mm(p);
  bad_fork_cleanup:
      free_pid(p->pid);
      free_thread_info(ti);
      free_task_struct(p);
      return ERR_PTR(retval);
  }

  // 复制内存空间（COW实现）
  static int copy_mm(unsigned long clone_flags, struct task_struct *p)
  {
      struct mm_struct *mm, *oldmm;

      oldmm = current->mm;

      // 内核线程没有用户空间
      if (!oldmm) {
          p->mm = NULL;
          p->active_mm = NULL;
          return 0;
      }

      // CLONE_VM: 共享地址空间（线程）
      if (clone_flags & CLONE_VM) {
          atomic_inc(&oldmm->mm_users);
          p->mm = oldmm;
          p->active_mm = oldmm;
          return 0;
      }

      // 创建新的mm_struct
      mm = allocate_mm();
      if (!mm)
          return -ENOMEM;

      // 复制mm内容
      memcpy(mm, oldmm, sizeof(*mm));
      atomic_set(&mm->mm_users, 1);
      atomic_set(&mm->mm_count, 1);

      // 分配新页表
      mm->pgd = pgd_alloc(mm);
      if (!mm->pgd) {
          free_mm(mm);
          return -ENOMEM;
      }

      // 复制所有VMA（使用COW）
      if (dup_mmap(mm, oldmm) < 0) {
          pgd_free(mm->pgd);
          free_mm(mm);
          return -ENOMEM;
      }

      p->mm = mm;
      p->active_mm = mm;

      return 0;
  }

  // 复制VMA并设置COW
  static int dup_mmap(struct mm_struct *mm, struct mm_struct *oldmm)
  {
      struct vm_area_struct *vma, *new_vma, **pprev;

      pprev = &mm->mmap;

      for (vma = oldmm->mmap; vma; vma = vma->vm_next) {
          // 分配新VMA
          new_vma = kmalloc(sizeof(*new_vma), GFP_KERNEL);
          if (!new_vma)
              return -ENOMEM;

          // 复制VMA内容
          *new_vma = *vma;
          new_vma->vm_mm = mm;

          // 复制页表项（设置为只读实现COW）
          copy_page_range(mm, oldmm, new_vma);

          // 链接到新mm
          *pprev = new_vma;
          pprev = &new_vma->vm_next;
          mm->map_count++;
      }

      return 0;
  }

  // 复制页表范围（COW核心）
  int copy_page_range(struct mm_struct *dst_mm, struct mm_struct *src_mm,
                      struct vm_area_struct *vma)
  {
      unsigned long addr = vma->vm_start;
      unsigned long end = vma->vm_end;

      for (; addr < end; addr += PAGE_SIZE) {
          pte_t *src_pte, *dst_pte;
          pte_t pte;

          src_pte = pte_offset(src_mm->pgd, addr);
          if (pte_none(*src_pte))
              continue;

          // 获取目标PTE
          dst_pte = pte_alloc(dst_mm->pgd, addr);
          if (!dst_pte)
              return -ENOMEM;

          pte = *src_pte;

          // 私有可写映射：设置为只读（COW）
          if (is_cow_mapping(vma->vm_flags) && pte_write(pte)) {
              // 清除写权限
              pte = pte_wrprotect(pte);
              set_pte(src_pte, pte);  // 父进程也变只读

              // 增加页引用计数
              struct page *page = pte_page(pte);
              get_page(page);
          }

          // 复制PTE到子进程
          set_pte(dst_pte, pte);
      }

      // 刷新TLB
      flush_tlb_mm(src_mm);

      return 0;
  }

  // COW页故障处理
  int do_cow_fault(struct vm_area_struct *vma, unsigned long address,
                   pte_t *page_table, pte_t orig_pte)
  {
      struct page *old_page, *new_page;
      pte_t entry;

      old_page = pte_page(orig_pte);

      // 如果只有一个引用，直接设置为可写
      if (page_count(old_page) == 1) {
          entry = pte_mkwrite(orig_pte);
          set_pte(page_table, entry);
          flush_tlb_page(vma, address);
          return 0;
      }

      // 分配新页
      new_page = alloc_page(GFP_USER);
      if (!new_page)
          return -ENOMEM;

      // 复制内容
      copy_page(page_address(new_page), page_address(old_page));

      // 更新页表
      entry = mk_pte(new_page, vma->vm_page_prot);
      entry = pte_mkwrite(entry);
      entry = pte_mkdirty(entry);
      set_pte(page_table, entry);

      // 释放旧页引用
      put_page(old_page);

      flush_tlb_page(vma, address);

      return 0;
  }

  // 复制线程上下文（RISC-V）
  int copy_thread(unsigned long clone_flags, unsigned long usp,
                  struct pt_regs *regs, struct task_struct *p)
  {
      struct pt_regs *childregs;
      struct thread_struct *thread = &p->thread;

      // 子进程的内核栈顶放pt_regs
      childregs = task_pt_regs(p);
      *childregs = *regs;

      // fork返回0给子进程
      childregs->a0 = 0;

      // 如果指定了栈，使用新栈
      if (usp)
          childregs->sp = usp;

      // 设置内核线程入口
      thread->ra = (unsigned long)ret_from_fork;
      thread->sp = (unsigned long)childregs;

      // s0-s11将在switch_to时恢复
      memset(thread->s, 0, sizeof(thread->s));

      return 0;
  }

  // fork系统调用
  SYSCALL_DEFINE0(fork)
  {
      struct pt_regs *regs = current_pt_regs();
      return do_fork(SIGCHLD, 0, regs);
  }

  // vfork系统调用（子进程先运行，共享地址空间直到exec）
  SYSCALL_DEFINE0(vfork)
  {
      struct pt_regs *regs = current_pt_regs();
      return do_fork(CLONE_VFORK | CLONE_VM | SIGCHLD, 0, regs);
  }

  // clone系统调用
  SYSCALL_DEFINE5(clone, unsigned long, clone_flags, unsigned long, newsp,
                  int __user *, parent_tidptr, unsigned long, tls,
                  int __user *, child_tidptr)
  {
      struct pt_regs *regs = current_pt_regs();
      return do_fork(clone_flags, newsp, regs);
  }

  // do_fork主函数
  long do_fork(unsigned long clone_flags, unsigned long stack_start,
               struct pt_regs *regs)
  {
      struct task_struct *p;

      p = copy_process(clone_flags, stack_start, regs);
      if (IS_ERR(p))
          return PTR_ERR(p);

      // 唤醒新进程
      wake_up_new_task(p);

      // 如果是vfork，等待子进程exec或exit
      if (clone_flags & CLONE_VFORK) {
          wait_for_vfork_done(p);
      }

      return p->pid;
  }

  ═══════════════════════════════════════════════════════════════
  任务2.4：进程调度器实现（2周）
  ═══════════════════════════════════════════════════════════════

  采用简化版O(1)调度器（后续可升级为CFS）：

  /* kernel/sched.c */

  // 运行队列（每CPU一个）
  struct rq {
      spinlock_t lock;

      unsigned long nr_running;       // 可运行进程数
      unsigned long nr_switches;      // 上下文切换次数

      struct task_struct *curr;       // 当前运行的进程
      struct task_struct *idle;       // idle进程

      // 优先级数组（O(1)调度）
      struct prio_array *active;      // 活跃队列
      struct prio_array *expired;     // 过期队列
      struct prio_array arrays[2];

      // 时钟
      u64 clock;                      // 队列时钟
      u64 clock_task;                 // 任务时钟
  };

  // 优先级数组
  #define MAX_PRIO    140
  #define BITMAP_SIZE ((MAX_PRIO + BITS_PER_LONG - 1) / BITS_PER_LONG)

  struct prio_array {
      unsigned int nr_active;         // 活跃任务数
      unsigned long bitmap[BITMAP_SIZE];  // 优先级位图
      struct list_head queue[MAX_PRIO];   // 每个优先级一个队列
  };

  // 全局运行队列（单CPU简化版）
  static struct rq runqueue;

  // 初始化调度器
  void __init sched_init(void)
  {
      struct rq *rq = &runqueue;
      int i;

      spin_lock_init(&rq->lock);
      rq->nr_running = 0;
      rq->nr_switches = 0;

      // 初始化优先级数组
      for (i = 0; i < 2; i++) {
          struct prio_array *array = &rq->arrays[i];
          int j;

          array->nr_active = 0;
          memset(array->bitmap, 0, sizeof(array->bitmap));
          for (j = 0; j < MAX_PRIO; j++)
              INIT_LIST_HEAD(&array->queue[j]);
      }

      rq->active = &rq->arrays[0];
      rq->expired = &rq->arrays[1];

      // 初始化idle进程（PID 0）
      rq->idle = &init_task;
      rq->curr = &init_task;
  }

  // 入队
  static void enqueue_task(struct rq *rq, struct task_struct *p)
  {
      struct prio_array *array = rq->active;
      int prio = p->prio;

      list_add_tail(&p->run_list, &array->queue[prio]);
      __set_bit(prio, array->bitmap);
      array->nr_active++;
      rq->nr_running++;
      p->on_rq = 1;
  }

  // 出队
  static void dequeue_task(struct rq *rq, struct task_struct *p)
  {
      struct prio_array *array = rq->active;
      int prio = p->prio;

      list_del(&p->run_list);
      if (list_empty(&array->queue[prio]))
          __clear_bit(prio, array->bitmap);
      array->nr_active--;
      rq->nr_running--;
      p->on_rq = 0;
  }

  // 选择下一个任务
  static struct task_struct *pick_next_task(struct rq *rq)
  {
      struct prio_array *array = rq->active;
      struct task_struct *next;
      int idx;

      // 如果活跃队列为空，交换active和expired
      if (unlikely(!array->nr_active)) {
          rq->active = rq->expired;
          rq->expired = array;
          array = rq->active;
      }

      // 找到最高优先级（最小数值）
      idx = find_first_bit(array->bitmap, MAX_PRIO);
      if (idx >= MAX_PRIO)
          return rq->idle;

      // 取队首任务
      next = list_first_entry(&array->queue[idx],
                              struct task_struct, run_list);
      return next;
  }

  // 定时器中断处理（每tick调用）
  void scheduler_tick(void)
  {
      struct rq *rq = &runqueue;
      struct task_struct *curr = rq->curr;

      spin_lock(&rq->lock);

      // 更新时钟
      rq->clock += TICK_NSEC;

      // 更新进程时间统计
      if (user_mode(get_irq_regs()))
          curr->utime++;
      else
          curr->stime++;

      // 减少时间片
      if (curr->time_slice > 0)
          curr->time_slice--;

      // 时间片用完
      if (curr->time_slice == 0) {
          // 重新计算时间片
          curr->time_slice = task_timeslice(curr);

          // 移到expired队列
          if (curr != rq->idle) {
              dequeue_task(rq, curr);
              // 添加到expired队列
              struct prio_array *array = rq->expired;
              list_add_tail(&curr->run_list, &array->queue[curr->prio]);
              __set_bit(curr->prio, array->bitmap);
              array->nr_active++;
              rq->nr_running++;
              curr->on_rq = 1;
          }

          // 设置需要调度标志
          set_tsk_need_resched(curr);
      }

      spin_unlock(&rq->lock);
  }

  // 主调度函数
  void schedule(void)
  {
      struct rq *rq = &runqueue;
      struct task_struct *prev, *next;
      unsigned long flags;

      local_irq_save(flags);
      spin_lock(&rq->lock);

      prev = rq->curr;

      // 如果当前进程不再运行，从队列移除
      if (prev->state != TASK_RUNNING && prev->on_rq) {
          dequeue_task(rq, prev);
      }

      // 选择下一个进程
      next = pick_next_task(rq);

      // 清除调度标志
      clear_tsk_need_resched(prev);

      if (prev != next) {
          rq->nr_switches++;
          rq->curr = next;

          // 上下文切换
          context_switch(rq, prev, next);
          // 注意：这里prev可能已经不是原来的进程了
      }

      spin_unlock(&rq->lock);
      local_irq_restore(flags);
  }

  // 上下文切换
  static void context_switch(struct rq *rq, struct task_struct *prev,
                             struct task_struct *next)
  {
      struct mm_struct *mm = next->mm;
      struct mm_struct *oldmm = prev->active_mm;

      // 切换地址空间
      if (!mm) {
          // 内核线程借用前一个进程的mm
          next->active_mm = oldmm;
          atomic_inc(&oldmm->mm_count);
      } else {
          switch_mm(oldmm, mm, next);
      }

      if (!prev->mm) {
          prev->active_mm = NULL;
          atomic_dec(&oldmm->mm_count);
      }

      // 切换寄存器上下文
      switch_to(prev, next);
  }

  // 唤醒进程
  int wake_up_process(struct task_struct *p)
  {
      struct rq *rq = &runqueue;
      unsigned long flags;

      spin_lock_irqsave(&rq->lock, flags);

      if (p->state == TASK_RUNNING) {
          spin_unlock_irqrestore(&rq->lock, flags);
          return 0;
      }

      p->state = TASK_RUNNING;
      enqueue_task(rq, p);

      // 如果比当前进程优先级高，设置调度标志
      if (p->prio < rq->curr->prio)
          set_tsk_need_resched(rq->curr);

      spin_unlock_irqrestore(&rq->lock, flags);
      return 1;
  }

  // 唤醒新创建的进程
  void wake_up_new_task(struct task_struct *p)
  {
      struct rq *rq = &runqueue;
      unsigned long flags;

      spin_lock_irqsave(&rq->lock, flags);

      p->state = TASK_RUNNING;
      p->time_slice = DEF_TIMESLICE;
      enqueue_task(rq, p);

      spin_unlock_irqrestore(&rq->lock, flags);
  }

  ═══════════════════════════════════════════════════════════════
  任务2.5：上下文切换实现（1周）
  ═══════════════════════════════════════════════════════════════

  /* arch/riscv64/kernel/switch.S */

  #include <asm/asm-offsets.h>

  /*
   * 上下文切换
   * void __switch_to(struct task_struct *prev, struct task_struct *next)
   *
   * a0 = prev task_struct
   * a1 = next task_struct
   */
  .global __switch_to
  .align 2
  __switch_to:
      // ========== 保存prev的上下文 ==========

      // 计算thread结构偏移
      // prev->thread 在 task_struct 中的偏移是 TASK_THREAD

      // 保存callee-saved寄存器到prev->thread
      sd ra,  TASK_THREAD_RA(a0)
      sd sp,  TASK_THREAD_SP(a0)
      sd s0,  TASK_THREAD_S0(a0)
      sd s1,  TASK_THREAD_S1(a0)
      sd s2,  TASK_THREAD_S2(a0)
      sd s3,  TASK_THREAD_S3(a0)
      sd s4,  TASK_THREAD_S4(a0)
      sd s5,  TASK_THREAD_S5(a0)
      sd s6,  TASK_THREAD_S6(a0)
      sd s7,  TASK_THREAD_S7(a0)
      sd s8,  TASK_THREAD_S8(a0)
      sd s9,  TASK_THREAD_S9(a0)
      sd s10, TASK_THREAD_S10(a0)
      sd s11, TASK_THREAD_S11(a0)

      // ========== 恢复next的上下文 ==========

      // 恢复callee-saved寄存器
      ld ra,  TASK_THREAD_RA(a1)
      ld sp,  TASK_THREAD_SP(a1)
      ld s0,  TASK_THREAD_S0(a1)
      ld s1,  TASK_THREAD_S1(a1)
      ld s2,  TASK_THREAD_S2(a1)
      ld s3,  TASK_THREAD_S3(a1)
      ld s4,  TASK_THREAD_S4(a1)
      ld s5,  TASK_THREAD_S5(a1)
      ld s6,  TASK_THREAD_S6(a1)
      ld s7,  TASK_THREAD_S7(a1)
      ld s8,  TASK_THREAD_S8(a1)
      ld s9,  TASK_THREAD_S9(a1)
      ld s10, TASK_THREAD_S10(a1)
      ld s11, TASK_THREAD_S11(a1)

      // 返回（ra已经是next的返回地址）
      ret

  /*
   * 切换地址空间
   * void switch_mm(struct mm_struct *prev_mm, struct mm_struct *next_mm,
   *                struct task_struct *next)
   */
  .global switch_mm
  switch_mm:
      // 加载next_mm->pgd
      ld t0, MM_PGD(a1)

      // 构造SATP值：MODE | ASID | PPN
      // SV39模式：MODE = 8
      srli t0, t0, PAGE_SHIFT      // 转换为PPN
      li t1, SATP_MODE_SV39
      or t0, t0, t1

      // 写入SATP
      csrw satp, t0

      // 刷新TLB
      sfence.vma

      ret

  /*
   * fork后子进程的入口
   */
  .global ret_from_fork
  ret_from_fork:
      // 调用schedule_tail（完成调度切换的收尾工作）
      call schedule_tail

      // 检查是否是内核线程
      ld t0, TASK_FLAGS(tp)       // tp = current
      andi t0, t0, PF_KTHREAD
      bnez t0, 1f

      // 用户进程：返回用户态
      j ret_to_user

  1:  // 内核线程：调用内核线程函数
      ld a0, TASK_THREAD_ARG(tp)  // 参数
      ld t0, TASK_THREAD_FUNC(tp) // 函数指针
      jalr t0
      j do_exit

  /* arch/riscv64/kernel/entry.S */

  /*
   * 返回用户态
   */
  .global ret_to_user
  ret_to_user:
      // 检查是否有待处理工作
      ld t0, TASK_FLAGS(tp)
      andi t0, t0, _TIF_WORK_MASK
      bnez t0, work_pending

  restore_all:
      // 恢复用户态寄存器
      RESTORE_ALL
      sret

  work_pending:
      // 检查是否需要调度
      andi t1, t0, _TIF_NEED_RESCHED
      beqz t1, 1f
      call schedule
      j ret_to_user

  1:  // 检查是否有待处理信号
      andi t1, t0, _TIF_SIGPENDING
      beqz t1, restore_all
      call do_signal
      j ret_to_user

  ═══════════════════════════════════════════════════════════════
  任务2.6：exec 实现 - ELF加载器（2周）
  ═══════════════════════════════════════════════════════════════

  exec 替换当前进程的地址空间为新程序：

  /* fs/exec.c */

  // 二进制格式处理器
  struct linux_binfmt {
      struct list_head lh;
      int (*load_binary)(struct linux_binprm *);
      int (*load_shlib)(struct file *);
      int (*core_dump)(struct coredump_params *);
  };

  // 二进制程序信息
  struct linux_binprm {
      char buf[BINPRM_BUF_SIZE];      // 文件头缓冲区
      struct file *file;              // 可执行文件
      int argc, envc;                 // 参数和环境变量数量
      const char *filename;           // 文件名
      const char *interp;             // 解释器路径
      unsigned long p;                // 当前栈指针
      unsigned long entry;            // 入口地址
      unsigned long loader;           // 动态链接器地址
      unsigned long exec;             // 执行地址
  };

  // execve系统调用
  SYSCALL_DEFINE3(execve, const char __user *, filename,
                  const char __user *const __user *, argv,
                  const char __user *const __user *, envp)
  {
      return do_execve(filename, argv, envp);
  }

  int do_execve(const char __user *filename,
                const char __user *const __user *argv,
                const char __user *const __user *envp)
  {
      struct linux_binprm *bprm;
      struct file *file;
      int retval;

      // 1. 分配bprm结构
      bprm = kzalloc(sizeof(*bprm), GFP_KERNEL);
      if (!bprm)
          return -ENOMEM;

      // 2. 打开可执行文件
      file = open_exec(filename);
      if (IS_ERR(file)) {
          retval = PTR_ERR(file);
          goto out_free;
      }
      bprm->file = file;
      bprm->filename = filename;

      // 3. 读取文件头
      retval = kernel_read(file, bprm->buf, BINPRM_BUF_SIZE, 0);
      if (retval < 0)
          goto out_close;

      // 4. 计算参数数量
      bprm->argc = count_argv(argv);
      bprm->envc = count_argv(envp);
      if (bprm->argc < 0 || bprm->envc < 0) {
          retval = -EFAULT;
          goto out_close;
      }

      // 5. 准备新的地址空间
      retval = bprm_mm_init(bprm);
      if (retval)
          goto out_close;

      // 6. 复制参数和环境变量到新栈
      retval = copy_strings(bprm->envc, envp, bprm);
      if (retval < 0)
          goto out_mm;

      retval = copy_strings(bprm->argc, argv, bprm);
      if (retval < 0)
          goto out_mm;

      // 7. 查找并调用合适的加载器
      retval = search_binary_handler(bprm);
      if (retval < 0)
          goto out_mm;

      // 成功，旧的mm已经被释放
      kfree(bprm);
      return retval;

  out_mm:
      // 清理新分配的mm
  out_close:
      fput(file);
  out_free:
      kfree(bprm);
      return retval;
  }

  // 搜索二进制处理器
  int search_binary_handler(struct linux_binprm *bprm)
  {
      struct linux_binfmt *fmt;
      int retval;

      // 检查ELF
      if (memcmp(bprm->buf, ELFMAG, SELFMAG) == 0) {
          return load_elf_binary(bprm);
      }

      // 检查脚本（#!）
      if (bprm->buf[0] == '#' && bprm->buf[1] == '!') {
          return load_script(bprm);
      }

      return -ENOEXEC;
  }

  ELF加载器详细实现：

  /* fs/binfmt_elf.c */

  // ELF文件头（64位）
  typedef struct {
      unsigned char e_ident[16];      // 魔数和标识
      Elf64_Half e_type;              // 文件类型
      Elf64_Half e_machine;           // 机器类型
      Elf64_Word e_version;           // 版本
      Elf64_Addr e_entry;             // 入口地址
      Elf64_Off e_phoff;              // 程序头偏移
      Elf64_Off e_shoff;              // 节头偏移
      Elf64_Word e_flags;             // 标志
      Elf64_Half e_ehsize;            // ELF头大小
      Elf64_Half e_phentsize;         // 程序头条目大小
      Elf64_Half e_phnum;             // 程序头数量
      Elf64_Half e_shentsize;         // 节头条目大小
      Elf64_Half e_shnum;             // 节头数量
      Elf64_Half e_shstrndx;          // 节名字符串表索引
  } Elf64_Ehdr;

  // 程序头
  typedef struct {
      Elf64_Word p_type;              // 段类型
      Elf64_Word p_flags;             // 段标志
      Elf64_Off p_offset;             // 文件偏移
      Elf64_Addr p_vaddr;             // 虚拟地址
      Elf64_Addr p_paddr;             // 物理地址
      Elf64_Xword p_filesz;           // 文件中大小
      Elf64_Xword p_memsz;            // 内存中大小
      Elf64_Xword p_align;            // 对齐
  } Elf64_Phdr;

  // 段类型
  #define PT_NULL     0   // 未使用
  #define PT_LOAD     1   // 可加载段
  #define PT_DYNAMIC  2   // 动态链接信息
  #define PT_INTERP   3   // 解释器路径
  #define PT_PHDR     6   // 程序头表

  // 加载ELF二进制
  static int load_elf_binary(struct linux_binprm *bprm)
  {
      Elf64_Ehdr *elf_ex = (Elf64_Ehdr *)bprm->buf;
      Elf64_Phdr *elf_phdata = NULL;
      struct mm_struct *mm;
      unsigned long load_addr = 0;
      unsigned long load_bias = 0;
      unsigned long start_code, end_code, start_data, end_data;
      unsigned long elf_bss, elf_brk;
      int retval, i;

      // 1. 验证ELF头
      if (memcmp(elf_ex->e_ident, ELFMAG, SELFMAG) != 0)
          return -ENOEXEC;

      // 检查架构
      if (elf_ex->e_machine != EM_RISCV)
          return -ENOEXEC;

      // 检查类型（可执行文件或共享库）
      if (elf_ex->e_type != ET_EXEC && elf_ex->e_type != ET_DYN)
          return -ENOEXEC;

      // 2. 读取程序头表
      elf_phdata = kmalloc(elf_ex->e_phnum * sizeof(Elf64_Phdr), GFP_KERNEL);
      if (!elf_phdata)
          return -ENOMEM;

      retval = kernel_read(bprm->file, elf_phdata,
                           elf_ex->e_phnum * sizeof(Elf64_Phdr),
                           elf_ex->e_phoff);
      if (retval < 0)
          goto out_free;

      // 3. 释放旧地址空间
      retval = flush_old_exec(bprm);
      if (retval)
          goto out_free;

      // 获取新的mm
      mm = current->mm;

      // 4. 初始化边界
      start_code = ~0UL;
      end_code = 0;
      start_data = 0;
      end_data = 0;
      elf_bss = 0;
      elf_brk = 0;

      // 5. 加载所有PT_LOAD段
      for (i = 0; i < elf_ex->e_phnum; i++) {
          Elf64_Phdr *phdr = &elf_phdata[i];
          unsigned long vaddr, size;
          unsigned int prot = 0;

          if (phdr->p_type != PT_LOAD)
              continue;

          // 计算保护标志
          if (phdr->p_flags & PF_R) prot |= PROT_READ;
          if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
          if (phdr->p_flags & PF_X) prot |= PROT_EXEC;

          vaddr = phdr->p_vaddr;
          size = phdr->p_memsz;

          // PIE/共享库：计算加载偏移
          if (elf_ex->e_type == ET_DYN && load_addr == 0) {
              load_bias = ELF_ET_DYN_BASE;
          }

          vaddr += load_bias;

          // 映射段
          retval = elf_map(bprm->file, vaddr, phdr, prot);
          if (retval < 0)
              goto out_free;

          // 更新边界
          if (phdr->p_flags & PF_X) {
              if (vaddr < start_code)
                  start_code = vaddr;
              if (vaddr + phdr->p_filesz > end_code)
                  end_code = vaddr + phdr->p_filesz;
          }
          if (start_data < vaddr)
              start_data = vaddr;
          if (end_data < vaddr + phdr->p_filesz)
              end_data = vaddr + phdr->p_filesz;
          if (elf_bss < vaddr + phdr->p_filesz)
              elf_bss = vaddr + phdr->p_filesz;
          if (elf_brk < vaddr + phdr->p_memsz)
              elf_brk = vaddr + phdr->p_memsz;

          if (load_addr == 0)
              load_addr = vaddr - phdr->p_offset;
      }

      // 6. 设置BSS（零初始化数据）
      if (elf_bss != elf_brk) {
          // BSS区域需要清零
          retval = set_brk(elf_bss, elf_brk);
          if (retval)
              goto out_free;
      }

      // 7. 设置mm边界
      mm->start_code = start_code;
      mm->end_code = end_code;
      mm->start_data = start_data;
      mm->end_data = end_data;
      mm->start_brk = elf_brk;
      mm->brk = elf_brk;

      // 8. 设置用户栈
      retval = setup_arg_pages(bprm, STACK_TOP);
      if (retval < 0)
          goto out_free;

      mm->start_stack = bprm->p;

      // 9. 设置auxv（musl需要）
      create_elf_tables(bprm, elf_ex, load_addr, load_bias);

      // 10. 设置入口点
      bprm->entry = elf_ex->e_entry + load_bias;

      // 11. 开始执行
      start_thread(current_pt_regs(), bprm->entry, bprm->p);

      retval = 0;

  out_free:
      kfree(elf_phdata);
      return retval;
  }

  // 映射ELF段
  static unsigned long elf_map(struct file *file, unsigned long addr,
                                Elf64_Phdr *phdr, int prot)
  {
      unsigned long map_addr;
      unsigned long size = phdr->p_filesz + (phdr->p_vaddr & ~PAGE_MASK);
      unsigned long off = phdr->p_offset - (phdr->p_vaddr & ~PAGE_MASK);

      addr = addr & PAGE_MASK;
      size = PAGE_ALIGN(size);

      // 使用mmap映射
      map_addr = do_mmap(file, addr, size, prot,
                         MAP_PRIVATE | MAP_FIXED, off);

      return map_addr;
  }

  // 创建ELF辅助向量（auxv）- musl启动必需
  static void create_elf_tables(struct linux_binprm *bprm,
                                 Elf64_Ehdr *exec, unsigned long load_addr,
                                 unsigned long load_bias)
  {
      unsigned long *sp = (unsigned long *)bprm->p;

      // auxv数组
      #define NEW_AUX_ENT(id, val) \
          do { *sp++ = id; *sp++ = val; } while(0)

      NEW_AUX_ENT(AT_PHDR, load_addr + exec->e_phoff);
      NEW_AUX_ENT(AT_PHENT, sizeof(Elf64_Phdr));
      NEW_AUX_ENT(AT_PHNUM, exec->e_phnum);
      NEW_AUX_ENT(AT_PAGESZ, PAGE_SIZE);
      NEW_AUX_ENT(AT_BASE, 0);  // 动态链接器基址
      NEW_AUX_ENT(AT_FLAGS, 0);
      NEW_AUX_ENT(AT_ENTRY, exec->e_entry + load_bias);
      NEW_AUX_ENT(AT_UID, current->uid);
      NEW_AUX_ENT(AT_EUID, current->euid);
      NEW_AUX_ENT(AT_GID, current->gid);
      NEW_AUX_ENT(AT_EGID, current->egid);
      NEW_AUX_ENT(AT_SECURE, 0);
      NEW_AUX_ENT(AT_RANDOM, sp);  // 16字节随机数地址
      NEW_AUX_ENT(AT_NULL, 0);

      bprm->p = (unsigned long)sp;
  }

  // 设置初始用户态寄存器
  void start_thread(struct pt_regs *regs, unsigned long pc, unsigned long sp)
  {
      memset(regs, 0, sizeof(*regs));
      regs->sepc = pc;            // 程序入口
      regs->sp = sp;              // 栈指针
      regs->sstatus = SR_SPIE | SR_SUM;  // 用户态，允许用户内存访问
  }

  ═══════════════════════════════════════════════════════════════
  任务2.7：wait/exit 实现（1周）
  ═══════════════════════════════════════════════════════════════

  /* kernel/exit.c */

  // 进程退出
  void do_exit(long code)
  {
      struct task_struct *tsk = current;

      // 设置退出标志
      tsk->flags |= PF_EXITING;
      tsk->exit_code = code;

      // 1. 释放内存
      exit_mm(tsk);

      // 2. 关闭文件
      exit_files(tsk);

      // 3. 释放文件系统信息
      exit_fs(tsk);

      // 4. 释放信号处理
      exit_sighand(tsk);

      // 5. 处理子进程（过继给init）
      exit_notify(tsk);

      // 6. 设置为僵尸状态
      tsk->state = TASK_ZOMBIE;

      // 7. 通知父进程
      if (tsk->parent)
          wake_up_interruptible(&tsk->parent->wait_chldexit);

      // 8. 调度离开
      schedule();

      // 永不返回
      BUG();
  }

  // 释放内存空间
  void exit_mm(struct task_struct *tsk)
  {
      struct mm_struct *mm = tsk->mm;

      if (!mm)
          return;

      tsk->mm = NULL;

      // 减少引用计数
      if (atomic_dec_and_test(&mm->mm_users)) {
          // 最后一个用户，释放mm
          exit_mmap(mm);
          pgd_free(mm->pgd);
          free_mm(mm);
      }
  }

  // 释放所有VMA
  void exit_mmap(struct mm_struct *mm)
  {
      struct vm_area_struct *vma, *next;

      for (vma = mm->mmap; vma; vma = next) {
          next = vma->vm_next;

          // 取消映射
          unmap_vma(mm, vma);

          // 释放VMA结构
          kfree(vma);
      }

      mm->mmap = NULL;
      mm->map_count = 0;
  }

  // 处理子进程
  static void exit_notify(struct task_struct *tsk)
  {
      struct task_struct *child, *tmp;

      // 将所有子进程过继给init
      list_for_each_entry_safe(child, tmp, &tsk->children, sibling) {
          child->parent = &init_task;
          child->real_parent = &init_task;
          list_move_tail(&child->sibling, &init_task.children);

          // 如果子进程是僵尸，通知init
          if (child->state == TASK_ZOMBIE)
              wake_up_interruptible(&init_task.wait_chldexit);
      }
  }

  // exit系统调用
  SYSCALL_DEFINE1(exit, int, error_code)
  {
      do_exit((error_code & 0xff) << 8);
      return 0;  // 永不返回
  }

  // exit_group系统调用（终止整个线程组）
  SYSCALL_DEFINE1(exit_group, int, error_code)
  {
      // 简化版：和exit相同
      do_exit((error_code & 0xff) << 8);
      return 0;
  }

  // wait系统调用实现
  SYSCALL_DEFINE4(wait4, pid_t, pid, int __user *, stat_addr,
                  int, options, struct rusage __user *, ru)
  {
      return do_wait(pid, stat_addr, options);
  }

  long do_wait(pid_t pid, int __user *stat_addr, int options)
  {
      struct task_struct *child;
      int retval;

  repeat:
      retval = -ECHILD;

      // 查找匹配的子进程
      list_for_each_entry(child, &current->children, sibling) {
          if (pid > 0 && child->pid != pid)
              continue;
          if (pid == 0 && child->pgrp != current->pgrp)
              continue;
          if (pid < -1 && child->pgrp != -pid)
              continue;

          // 找到子进程
          if (child->state == TASK_ZOMBIE) {
              // 子进程已退出
              retval = child->pid;

              // 返回退出状态
              if (stat_addr)
                  put_user(child->exit_code, stat_addr);

              // 释放子进程
              release_task(child);
              return retval;
          }

          // 有子进程但未退出
          retval = 0;
      }

      if (retval == -ECHILD)
          return -ECHILD;

      // WNOHANG：不阻塞
      if (options & WNOHANG)
          return 0;

      // 等待子进程退出
      retval = wait_event_interruptible(current->wait_chldexit,
                                         has_zombie_child(current));
      if (retval == -ERESTARTSYS)
          return retval;

      goto repeat;
  }

  // 释放僵尸进程
  void release_task(struct task_struct *p)
  {
      // 从父进程的子进程链表移除
      list_del(&p->sibling);

      // 从全局进程链表移除
      list_del(&p->tasks);

      // 释放PID
      free_pid(p->pid);

      // 释放内核栈
      free_thread_info(task_thread_info(p));

      // 释放task_struct
      free_task_struct(p);
  }

  ═══════════════════════════════════════════════════════════════
  任务2.8：idle进程和init进程（1周）
  ═══════════════════════════════════════════════════════════════

  系统启动后的进程结构：

  PID 0: idle（swapper）
    │
    └── PID 1: init
          │
          ├── PID 2: shell
          │     │
          │     └── PID 3: user_program
          │
          └── PID 4: other_daemon

  /* init/main.c */

  // init_task是idle进程（PID 0）的task_struct
  // 静态初始化，不是fork出来的
  struct task_struct init_task = {
      .state          = TASK_RUNNING,
      .flags          = PF_KTHREAD,
      .prio           = MAX_PRIO - 1,
      .static_prio    = MAX_PRIO - 1,
      .policy         = SCHED_NORMAL,
      .pid            = 0,
      .tgid           = 0,
      .comm           = "swapper",
      .mm             = NULL,  // 内核线程
      .active_mm      = &init_mm,
      .tasks          = LIST_HEAD_INIT(init_task.tasks),
      .children       = LIST_HEAD_INIT(init_task.children),
      .sibling        = LIST_HEAD_INIT(init_task.sibling),
  };

  // 内核地址空间
  struct mm_struct init_mm = {
      .pgd            = swapper_pg_dir,
      .mm_users       = ATOMIC_INIT(2),
      .mm_count       = ATOMIC_INIT(1),
  };

  // 创建init进程
  static int kernel_init(void *unused)
  {
      // 现在运行在PID 1进程中

      // 等待所有初始化完成
      wait_for_initramfs();

      // 尝试执行init程序
      if (execute_command) {
          // 命令行指定的init
          run_init_process(execute_command);
      }

      // 尝试标准init路径
      run_init_process("/sbin/init");
      run_init_process("/etc/init");
      run_init_process("/bin/init");
      run_init_process("/bin/sh");

      panic("No init found!");
      return 0;
  }

  static void run_init_process(const char *init_filename)
  {
      const char *argv[] = { init_filename, NULL };
      const char *envp[] = { "HOME=/", "PATH=/bin:/sbin", NULL };

      kernel_execve(init_filename, argv, envp);
  }

  // 主初始化函数
  asmlinkage void __init start_kernel(void)
  {
      // ... 各种初始化 ...

      // 初始化调度器
      sched_init();

      // 创建内核线程
      kernel_thread(kernel_init, NULL, CLONE_FS);

      // 变成idle进程
      cpu_idle();
  }

  // idle循环
  void cpu_idle(void)
  {
      while (1) {
          // 如果有可运行进程，调度
          while (!need_resched()) {
              // 低功耗等待中断
              asm volatile ("wfi");
          }
          schedule();
      }
  }

  // 创建内核线程
  pid_t kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
  {
      struct pt_regs regs;

      memset(&regs, 0, sizeof(regs));
      regs.a0 = (unsigned long)fn;
      regs.a1 = (unsigned long)arg;

      return do_fork(flags | CLONE_VM | CLONE_UNTRACED, 0, &regs);
  }

  阶段2验收标准：
  - ⬜ 能创建新进程（fork），支持COW
  - ⬜ 能加载并执行ELF程序（exec）
  - ⬜ 进程能正常调度和切换
  - ⬜ 父子进程关系正确
  - ⬜ wait/exit正常工作
  - ⬜ idle进程和init进程正常运行
  - ⬜ 用户态程序能够运行

  ---
  阶段3：系统调用层（2-3个月）

  任务3.1：实现系统调用框架（2周）

  步骤：

  1. 定义系统调用表
  /* arch/riscv64/kernel/syscall_table.S */

  ENTRY(sys_call_table)
      .quad sys_io_setup              // 0
      .quad sys_io_destroy
      .quad sys_io_submit
      .quad sys_io_cancel
      .quad sys_io_getevents
      .quad sys_setxattr              // 5
      // ... 总共约300个系统调用
      .quad sys_openat                // 56
      .quad sys_close                 // 57
      .quad sys_read                  // 63
      .quad sys_write                 // 64
      .quad sys_exit                  // 93
      .quad sys_fork                  // 220 (RISC-V)
      // ...
  END(sys_call_table)
  2. 实现系统调用入口
  /* arch/riscv64/kernel/entry.S */

  ENTRY(handle_syscall)
      // 保存用户态寄存器
      save_all_regs

      // 获取系统调用号（a7寄存器）
      ld t0, PT_A7(sp)

      // 检查系统调用号是否有效
      li t1, __NR_syscalls
      bgeu t0, t1, syscall_enosys

      // 调用系统调用
      slli t0, t0, 3          // t0 *= 8
      la t1, sys_call_table
      add t1, t1, t0
      ld t1, 0(t1)
      jalr t1

      // 恢复寄存器并返回用户态
      restore_all_regs
      sret

  syscall_enosys:
      li a0, -ENOSYS
      j syscall_return
  ENDPROC(handle_syscall)
  3. 定义系统调用宏
  /* include/minix/syscalls.h */

  #define SYSCALL_DEFINE0(name) \
      asmlinkage long sys_##name(void)

  #define SYSCALL_DEFINE1(name, type1, arg1) \
      asmlinkage long sys_##name(type1 arg1)

  #define SYSCALL_DEFINE2(name, type1, arg1, type2, arg2) \
      asmlinkage long sys_##name(type1 arg1, type2 arg2)

  // 类似地定义SYSCALL_DEFINE3到SYSCALL_DEFINE6

  任务3.2：实现核心系统调用（4周）

  进程管理（1周）
  /* kernel/fork.c */
  SYSCALL_DEFINE0(fork) { ... }
  SYSCALL_DEFINE0(vfork) { ... }
  SYSCALL_DEFINE5(clone, ...) { ... }

  /* kernel/exec.c */
  SYSCALL_DEFINE3(execve, const char __user *, filename,
                  const char __user *const __user *, argv,
                  const char __user *const __user *, envp) { ... }

  /* kernel/exit.c */
  SYSCALL_DEFINE1(exit, int, error_code) { ... }
  SYSCALL_DEFINE4(wait4, ...) { ... }
  SYSCALL_DEFINE2(waitpid, ...) { ... }

  /* kernel/sys.c */
  SYSCALL_DEFINE0(getpid) { return current->pid; }
  SYSCALL_DEFINE0(getppid) { return current->ppid; }
  SYSCALL_DEFINE0(getuid) { return current->uid; }
  SYSCALL_DEFINE0(geteuid) { return current->euid; }

  文件系统（2周）
  /* fs/open.c */
  SYSCALL_DEFINE3(open, const char __user *, filename,
                  int, flags, umode_t, mode)
  {
      return do_sys_open(AT_FDCWD, filename, flags, mode);
  }

  SYSCALL_DEFINE4(openat, int, dfd, const char __user *, filename,
                  int, flags, umode_t, mode)
  {
      return do_sys_open(dfd, filename, flags, mode);
  }

  SYSCALL_DEFINE1(close, unsigned int, fd)
  {
      struct file *file = fget(fd);
      if (!file) return -EBADF;

      fput(file);
      return 0;
  }

  /* fs/read_write.c */
  SYSCALL_DEFINE3(read, unsigned int, fd,
                  char __user *, buf, size_t, count)
  {
      struct file *file = fget(fd);
      if (!file) return -EBADF;

      ssize_t ret = vfs_read(file, buf, count, &file->f_pos);
      fput(file);
      return ret;
  }

  SYSCALL_DEFINE3(write, unsigned int, fd,
                  const char __user *, buf, size_t, count)
  {
      struct file *file = fget(fd);
      if (!file) return -EBADF;

      ssize_t ret = vfs_write(file, buf, count, &file->f_pos);
      fput(file);
      return ret;
  }

  SYSCALL_DEFINE3(lseek, unsigned int, fd,
                  off_t, offset, unsigned int, whence) { ... }

  /* fs/stat.c */
  SYSCALL_DEFINE2(stat, const char __user *, filename,
                  struct stat __user *, statbuf) { ... }
  SYSCALL_DEFINE2(fstat, unsigned int, fd,
                  struct stat __user *, statbuf) { ... }

  /* fs/namei.c */
  SYSCALL_DEFINE2(mkdir, const char __user *, pathname, umode_t, mode) { ... }
  SYSCALL_DEFINE1(rmdir, const char __user *, pathname) { ... }
  SYSCALL_DEFINE1(unlink, const char __user *, pathname) { ... }
  SYSCALL_DEFINE2(rename, const char __user *, oldname,
                  const char __user *, newname) { ... }
  SYSCALL_DEFINE2(link, const char __user *, oldname,
                  const char __user *, newname) { ... }
  SYSCALL_DEFINE2(symlink, const char __user *, oldname,
                  const char __user *, newname) { ... }

  内存管理（1周）
  /* mm/mmap.c */
  SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
                  unsigned long, prot, unsigned long, flags,
                  unsigned long, fd, unsigned long, offset)
  {
      return do_mmap(addr, len, prot, flags, fd, offset);
  }

  SYSCALL_DEFINE2(munmap, unsigned long, addr, size_t, len)
  {
      return do_munmap(current->mm, addr, len);
  }

  SYSCALL_DEFINE1(brk, unsigned long, brk)
  {
      return do_brk(brk);
  }

  信号（后续阶段实现）

  任务3.3：实现文件描述符管理（1周）

  /* kernel/fd.c */

  struct files_struct {
      atomic_t count;
      spinlock_t file_lock;
      struct fdtable *fdt;
      struct fdtable fdtab;
      struct file *fd_array[NR_OPEN_DEFAULT];  // 默认64个
  };

  struct fdtable {
      unsigned int max_fds;
      struct file **fd;       // 文件指针数组
      unsigned long *open_fds; // bitmap：哪些fd在使用
      unsigned long *close_on_exec;
  };

  int get_unused_fd(void)
  {
      struct files_struct *files = current->files;
      int fd;

      spin_lock(&files->file_lock);
      fd = find_first_zero_bit(files->fdt->open_fds,
                                files->fdt->max_fds);
      if (fd >= files->fdt->max_fds) {
          // 需要扩展fd表
          fd = expand_files(files, fd);
      }
      set_bit(fd, files->fdt->open_fds);
      spin_unlock(&files->file_lock);

      return fd;
  }

  void fd_install(unsigned int fd, struct file *file)
  {
      struct files_struct *files = current->files;
      files->fdt->fd[fd] = file;
  }

  struct file *fget(unsigned int fd)
  {
      struct files_struct *files = current->files;
      struct file *file;

      if (fd >= files->fdt->max_fds)
          return NULL;

      file = files->fdt->fd[fd];
      if (file)
          atomic_inc(&file->f_count);

      return file;
  }

  任务3.4：用户态/内核态切换（2周）

  /* arch/riscv64/kernel/traps.c */

  void do_trap_user(struct pt_regs *regs)
  {
      unsigned long cause = csr_read(scause);

      switch (cause) {
      case EXC_SYSCALL:
          handle_syscall(regs);
          break;
      case EXC_INST_PAGE_FAULT:
      case EXC_LOAD_PAGE_FAULT:
      case EXC_STORE_PAGE_FAULT:
          do_page_fault(regs);
          break;
      case EXC_INST_ILLEGAL:
          do_trap_illegal_insn(regs);
          break;
      default:
          do_trap_unknown(regs);
      }
  }

  // 返回用户态前的检查
  void exit_to_user_mode(struct pt_regs *regs)
  {
      // 检查是否有待处理的信号
      if (test_thread_flag(TIF_SIGPENDING)) {
          do_signal(regs);
      }

      // 检查是否需要调度
      if (test_thread_flag(TIF_NEED_RESCHED)) {
          schedule();
      }
  }

  阶段3验收标准：
  - ⬜ 系统调用机制正常工作
  - ⬜ 核心系统调用实现完成
  - ⬜ 用户态程序能通过系统调用与内核交互
  - ⬜ 文件描述符管理正确

  ---
  阶段4：C标准库移植 - musl libc（2-3个月）

  任务4.1：为什么选择 musl 而非 newlib

  ═══════════════════════════════════════════════════════════════
  选项对比（重新评估）
  ═══════════════════════════════════════════════════════════════

  | C库     | 优点                           | 缺点                    | 推荐度   |
  |--------|------------------------------|------------------------|---------|
  | musl   | 完整POSIX/Linux兼容，代码简洁，静态链接友好 | 需要更多系统调用支持            | ⭐⭐⭐⭐⭐ |
  | newlib | 体积小，嵌入式常用               | 不完全兼容POSIX，API有差异     | ⭐⭐⭐   |
  | glibc  | 功能最完整                      | 体积大，难以移植，动态链接依赖重    | ⭐⭐    |

  决定：直接使用 musl libc

  musl 的核心优势：
  1. 真正的 POSIX/Linux 兼容 - 系统调用接口与 Linux 完全一致
  2. 代码简洁清晰 - 约 10 万行代码，易于理解和调试
  3. 静态链接友好 - 生成的二进制文件独立运行，适合嵌入式
  4. 安全性高 - 注重安全设计，无 glibc 历史包袱
  5. 一步到位 - 不需要先移植 newlib 再迁移

  为什么 MinixRV64 选择 Linux syscall ABI？
  ┌─────────────────────────────────────────────────────────────┐
  │  MinixRV64 = Minix 微内核设计 + Linux 用户态兼容            │
  │                                                             │
  │  内核层面（Minix 风格）：                                   │
  │    • 微内核架构，核心精简                                   │
  │    • 服务进程隔离（VFS、PM、驱动等）                        │
  │    • 消息传递 IPC                                           │
  │                                                             │
  │  用户态接口（Linux 兼容）：                                 │
  │    • 使用 Linux RISC-V 系统调用号                           │
  │    • 支持 musl libc 编译的程序                              │
  │    • 标准 ELF 格式                                          │
  │                                                             │
  │  好处：                                                     │
  │    • 可直接运行大量现有 Linux/musl 程序                     │
  │    • 无需为每个程序单独移植                                 │
  │    • 学习时可对比 Minix 和 Linux 的设计差异                 │
  └─────────────────────────────────────────────────────────────┘

  musl 对内核的要求（这正是我们需要实现的）：
  - 约 50-60 个核心系统调用（比 newlib 的 20 个多，但功能完整）
  - Linux 兼容的系统调用号（RISC-V 使用统一的 Linux syscall ABI）
  - 正确的 signal 处理
  - 线程支持（可选，单线程程序可以先不实现）

  ═══════════════════════════════════════════════════════════════
  任务4.2：musl 移植的系统调用需求分析
  ═══════════════════════════════════════════════════════════════

  musl 所需的最小系统调用集（分优先级）：

  【P0 - 绝对必需】启动和基本 I/O
  ┌─────────────────────────────────────────────────────────────┐
  │ SYS_exit          (93)   - 进程退出                         │
  │ SYS_exit_group    (94)   - 线程组退出                       │
  │ SYS_write         (64)   - 写文件/stdout                    │
  │ SYS_writev        (66)   - 向量写（printf 需要）            │
  │ SYS_read          (63)   - 读文件/stdin                     │
  │ SYS_brk           (214)  - 堆内存管理                       │
  │ SYS_mmap          (222)  - 内存映射（malloc 大块分配）       │
  │ SYS_munmap        (215)  - 解除映射                         │
  │ SYS_close         (57)   - 关闭文件描述符                   │
  │ SYS_openat        (56)   - 打开文件（现代接口）              │
  │ SYS_fstat         (80)   - 获取文件状态                     │
  └─────────────────────────────────────────────────────────────┘

  【P1 - 进程管理】fork/exec 需要
  ┌─────────────────────────────────────────────────────────────┐
  │ SYS_clone         (220)  - 创建进程/线程                    │
  │ SYS_execve        (221)  - 执行程序                         │
  │ SYS_wait4         (260)  - 等待子进程                       │
  │ SYS_getpid        (172)  - 获取进程 ID                      │
  │ SYS_getppid       (173)  - 获取父进程 ID                    │
  │ SYS_getuid        (174)  - 获取用户 ID                      │
  │ SYS_geteuid       (175)  - 获取有效用户 ID                  │
  │ SYS_getgid        (176)  - 获取组 ID                        │
  │ SYS_getegid       (177)  - 获取有效组 ID                    │
  │ SYS_gettid        (178)  - 获取线程 ID                      │
  │ SYS_set_tid_address (96) - 设置 TID 地址（线程）            │
  └─────────────────────────────────────────────────────────────┘

  【P2 - 文件系统】完整文件操作
  ┌─────────────────────────────────────────────────────────────┐
  │ SYS_lseek         (62)   - 文件定位                         │
  │ SYS_ioctl         (29)   - 设备控制                         │
  │ SYS_readv         (65)   - 向量读                           │
  │ SYS_pread64       (67)   - 位置读                           │
  │ SYS_pwrite64      (68)   - 位置写                           │
  │ SYS_fcntl         (25)   - 文件控制                         │
  │ SYS_dup           (23)   - 复制文件描述符                   │
  │ SYS_dup3          (24)   - 复制文件描述符（带标志）          │
  │ SYS_mkdirat       (34)   - 创建目录                         │
  │ SYS_unlinkat      (35)   - 删除文件                         │
  │ SYS_renameat      (38)   - 重命名                           │
  │ SYS_fstatat       (79)   - 获取文件状态（相对路径）          │
  │ SYS_readlinkat    (78)   - 读取符号链接                     │
  │ SYS_faccessat     (48)   - 检查访问权限                     │
  │ SYS_getcwd        (17)   - 获取当前目录                     │
  │ SYS_chdir         (49)   - 切换目录                         │
  │ SYS_fchdir        (50)   - 切换目录（fd）                   │
  │ SYS_getdents64    (61)   - 读取目录项                       │
  │ SYS_pipe2         (59)   - 创建管道                         │
  └─────────────────────────────────────────────────────────────┘

  【P3 - 信号】信号处理
  ┌─────────────────────────────────────────────────────────────┐
  │ SYS_rt_sigaction  (134)  - 设置信号处理                     │
  │ SYS_rt_sigprocmask(135)  - 信号屏蔽                         │
  │ SYS_rt_sigreturn  (139)  - 信号返回                         │
  │ SYS_kill          (129)  - 发送信号                         │
  │ SYS_tgkill        (131)  - 发送信号给线程                   │
  └─────────────────────────────────────────────────────────────┘

  【P4 - 时间】时间相关
  ┌─────────────────────────────────────────────────────────────┐
  │ SYS_clock_gettime (113)  - 获取时钟                         │
  │ SYS_nanosleep     (101)  - 睡眠                             │
  │ SYS_gettimeofday  (169)  - 获取时间                         │
  └─────────────────────────────────────────────────────────────┘

  【P5 - 可选/线程】多线程支持（可以后续添加）
  ┌─────────────────────────────────────────────────────────────┐
  │ SYS_futex         (98)   - 快速用户空间互斥                 │
  │ SYS_mprotect      (226)  - 修改内存保护                     │
  │ SYS_madvise       (233)  - 内存建议                         │
  └─────────────────────────────────────────────────────────────┘

  ═══════════════════════════════════════════════════════════════
  任务4.3：内核系统调用表实现
  ═══════════════════════════════════════════════════════════════

  RISC-V Linux 系统调用约定：
  - a7 = 系统调用号
  - a0-a5 = 参数
  - a0 = 返回值
  - 使用 ecall 指令触发

  /* arch/riscv64/kernel/syscall_table.c */

  #include <minix/syscall.h>

  // 系统调用号定义（遵循 Linux RISC-V ABI）
  #define __NR_getcwd          17
  #define __NR_dup             23
  #define __NR_dup3            24
  #define __NR_fcntl           25
  #define __NR_ioctl           29
  #define __NR_mkdirat         34
  #define __NR_unlinkat        35
  #define __NR_renameat        38
  #define __NR_faccessat       48
  #define __NR_chdir           49
  #define __NR_fchdir          50
  #define __NR_openat          56
  #define __NR_close           57
  #define __NR_pipe2           59
  #define __NR_getdents64      61
  #define __NR_lseek           62
  #define __NR_read            63
  #define __NR_write           64
  #define __NR_readv           65
  #define __NR_writev          66
  #define __NR_pread64         67
  #define __NR_pwrite64        68
  #define __NR_readlinkat      78
  #define __NR_fstatat         79
  #define __NR_fstat           80
  #define __NR_exit            93
  #define __NR_exit_group      94
  #define __NR_set_tid_address 96
  #define __NR_futex           98
  #define __NR_nanosleep       101
  #define __NR_clock_gettime   113
  #define __NR_kill            129
  #define __NR_tgkill          131
  #define __NR_rt_sigaction    134
  #define __NR_rt_sigprocmask  135
  #define __NR_rt_sigreturn    139
  #define __NR_gettimeofday    169
  #define __NR_getpid          172
  #define __NR_getppid         173
  #define __NR_getuid          174
  #define __NR_geteuid         175
  #define __NR_getgid          176
  #define __NR_getegid         177
  #define __NR_gettid          178
  #define __NR_brk             214
  #define __NR_munmap          215
  #define __NR_clone           220
  #define __NR_execve          221
  #define __NR_mmap            222
  #define __NR_mprotect        226
  #define __NR_madvise         233
  #define __NR_wait4           260

  #define __NR_syscalls        512  // 系统调用表大小

  typedef long (*syscall_fn_t)(long, long, long, long, long, long);

  // 未实现的系统调用返回 -ENOSYS
  static long sys_ni_syscall(long a0, long a1, long a2,
                             long a3, long a4, long a5)
  {
      return -ENOSYS;
  }

  // 系统调用表
  syscall_fn_t sys_call_table[__NR_syscalls] = {
      [0 ... __NR_syscalls-1] = sys_ni_syscall,

      // P0 - 必需
      [__NR_exit]           = (syscall_fn_t)sys_exit,
      [__NR_exit_group]     = (syscall_fn_t)sys_exit_group,
      [__NR_read]           = (syscall_fn_t)sys_read,
      [__NR_write]          = (syscall_fn_t)sys_write,
      [__NR_writev]         = (syscall_fn_t)sys_writev,
      [__NR_openat]         = (syscall_fn_t)sys_openat,
      [__NR_close]          = (syscall_fn_t)sys_close,
      [__NR_fstat]          = (syscall_fn_t)sys_fstat,
      [__NR_brk]            = (syscall_fn_t)sys_brk,
      [__NR_mmap]           = (syscall_fn_t)sys_mmap,
      [__NR_munmap]         = (syscall_fn_t)sys_munmap,

      // P1 - 进程
      [__NR_clone]          = (syscall_fn_t)sys_clone,
      [__NR_execve]         = (syscall_fn_t)sys_execve,
      [__NR_wait4]          = (syscall_fn_t)sys_wait4,
      [__NR_getpid]         = (syscall_fn_t)sys_getpid,
      [__NR_getppid]        = (syscall_fn_t)sys_getppid,
      [__NR_getuid]         = (syscall_fn_t)sys_getuid,
      [__NR_geteuid]        = (syscall_fn_t)sys_geteuid,
      [__NR_getgid]         = (syscall_fn_t)sys_getgid,
      [__NR_getegid]        = (syscall_fn_t)sys_getegid,
      [__NR_gettid]         = (syscall_fn_t)sys_gettid,
      [__NR_set_tid_address]= (syscall_fn_t)sys_set_tid_address,

      // P2 - 文件系统
      [__NR_lseek]          = (syscall_fn_t)sys_lseek,
      [__NR_ioctl]          = (syscall_fn_t)sys_ioctl,
      [__NR_readv]          = (syscall_fn_t)sys_readv,
      [__NR_pread64]        = (syscall_fn_t)sys_pread64,
      [__NR_pwrite64]       = (syscall_fn_t)sys_pwrite64,
      [__NR_fcntl]          = (syscall_fn_t)sys_fcntl,
      [__NR_dup]            = (syscall_fn_t)sys_dup,
      [__NR_dup3]           = (syscall_fn_t)sys_dup3,
      [__NR_mkdirat]        = (syscall_fn_t)sys_mkdirat,
      [__NR_unlinkat]       = (syscall_fn_t)sys_unlinkat,
      [__NR_renameat]       = (syscall_fn_t)sys_renameat,
      [__NR_fstatat]        = (syscall_fn_t)sys_fstatat,
      [__NR_readlinkat]     = (syscall_fn_t)sys_readlinkat,
      [__NR_faccessat]      = (syscall_fn_t)sys_faccessat,
      [__NR_getcwd]         = (syscall_fn_t)sys_getcwd,
      [__NR_chdir]          = (syscall_fn_t)sys_chdir,
      [__NR_fchdir]         = (syscall_fn_t)sys_fchdir,
      [__NR_getdents64]     = (syscall_fn_t)sys_getdents64,
      [__NR_pipe2]          = (syscall_fn_t)sys_pipe2,

      // P3 - 信号
      [__NR_rt_sigaction]   = (syscall_fn_t)sys_rt_sigaction,
      [__NR_rt_sigprocmask] = (syscall_fn_t)sys_rt_sigprocmask,
      [__NR_rt_sigreturn]   = (syscall_fn_t)sys_rt_sigreturn,
      [__NR_kill]           = (syscall_fn_t)sys_kill,
      [__NR_tgkill]         = (syscall_fn_t)sys_tgkill,

      // P4 - 时间
      [__NR_clock_gettime]  = (syscall_fn_t)sys_clock_gettime,
      [__NR_nanosleep]      = (syscall_fn_t)sys_nanosleep,
      [__NR_gettimeofday]   = (syscall_fn_t)sys_gettimeofday,

      // P5 - 线程/内存
      [__NR_futex]          = (syscall_fn_t)sys_futex,
      [__NR_mprotect]       = (syscall_fn_t)sys_mprotect,
      [__NR_madvise]        = (syscall_fn_t)sys_madvise,
  };

  ═══════════════════════════════════════════════════════════════
  任务4.4：musl 交叉编译工具链构建
  ═══════════════════════════════════════════════════════════════

  方案A：使用 musl-cross-make（推荐）

  # 克隆 musl-cross-make
  git clone https://github.com/richfelker/musl-cross-make
  cd musl-cross-make

  # 创建配置文件
  cat > config.mak << 'EOF'
  TARGET = riscv64-linux-musl
  OUTPUT = /opt/cross/riscv64-linux-musl

  # GCC 版本
  GCC_VER = 13.2.0
  MUSL_VER = 1.2.4
  BINUTILS_VER = 2.41

  # 优化选项
  COMMON_CONFIG += --disable-nls
  GCC_CONFIG += --enable-languages=c,c++
  GCC_CONFIG += --disable-libquadmath
  GCC_CONFIG += --disable-decimal-float
  EOF

  # 构建（需要较长时间）
  make -j$(nproc)
  make install

  # 添加到 PATH
  export PATH=/opt/cross/riscv64-linux-musl/bin:$PATH

  方案B：直接下载预编译工具链

  # 从 musl.cc 下载预编译工具链
  wget https://musl.cc/riscv64-linux-musl-cross.tgz
  tar xf riscv64-linux-musl-cross.tgz
  export PATH=$PWD/riscv64-linux-musl-cross/bin:$PATH

  验证：
  riscv64-linux-musl-gcc --version
  # riscv64-linux-musl-gcc (GCC) 13.x.x

  ═══════════════════════════════════════════════════════════════
  任务4.5：用户空间启动代码（crt）
  ═══════════════════════════════════════════════════════════════

  musl 自带 crt（crt1.o, crti.o, crtn.o），但我们需要确保内核
  正确设置用户态栈和寄存器。

  内核设置用户态入口（在 execve 中）：

  /* kernel/exec.c */

  int setup_user_stack(struct task_struct *task,
                       int argc, char **argv, char **envp)
  {
      unsigned long sp = task->mm->start_stack;
      unsigned long *stack = (unsigned long *)sp;

      // Linux/musl ABI 要求的栈布局：
      //
      // 高地址
      // ┌─────────────────────────────┐
      // │ 环境变量字符串              │
      // │ 参数字符串                  │
      // │ 填充（16字节对齐）          │
      // ├─────────────────────────────┤
      // │ auxv[n] = {AT_NULL, 0}      │
      // │ ...                         │
      // │ auxv[0] = {type, value}     │
      // ├─────────────────────────────┤
      // │ NULL                        │
      // │ envp[n-1]                   │
      // │ ...                         │
      // │ envp[0]                     │
      // ├─────────────────────────────┤
      // │ NULL                        │
      // │ argv[n-1]                   │
      // │ ...                         │
      // │ argv[0]                     │
      // ├─────────────────────────────┤
      // │ argc                        │ ← sp 入口点
      // └─────────────────────────────┘
      // 低地址

      // 1. 复制字符串到栈顶
      // 2. 设置 argv[] 指针数组
      // 3. 设置 envp[] 指针数组
      // 4. 设置 auxv（辅助向量）- musl 需要这个！

      // auxv 是 musl 必需的：
      Elf64_auxv_t auxv[] = {
          {AT_PAGESZ, PAGE_SIZE},           // 页大小
          {AT_PHDR, task->mm->elf_phdr},    // 程序头地址
          {AT_PHENT, sizeof(Elf64_Phdr)},   // 程序头条目大小
          {AT_PHNUM, task->mm->elf_phnum},  // 程序头数量
          {AT_ENTRY, task->mm->elf_entry},  // 入口点
          {AT_UID, 0}, {AT_EUID, 0},        // UID
          {AT_GID, 0}, {AT_EGID, 0},        // GID
          {AT_SECURE, 0},                    // 安全模式
          {AT_RANDOM, random_ptr},           // 16字节随机数地址
          {AT_NULL, 0}                       // 结束标记
      };

      // 写入栈...
      return 0;
  }

  ═══════════════════════════════════════════════════════════════
  任务4.6：编译用户程序
  ═══════════════════════════════════════════════════════════════

  静态链接 Hello World：

  /* userspace/hello.c */
  #include <stdio.h>
  #include <unistd.h>

  int main(int argc, char **argv)
  {
      printf("Hello from MinixRV64 with musl!\n");
      printf("PID: %d\n", getpid());

      for (int i = 0; i < argc; i++) {
          printf("argv[%d] = %s\n", i, argv[i]);
      }

      return 0;
  }

  编译（静态链接）：
  riscv64-linux-musl-gcc -static -o hello hello.c

  查看依赖的系统调用（用于测试）：
  riscv64-linux-musl-objdump -d hello | grep ecall

  最小测试程序（不依赖 libc）：

  /* userspace/minimal.S */
  .global _start
  _start:
      # write(1, msg, 14)
      li a7, 64          # SYS_write
      li a0, 1           # fd = stdout
      la a1, msg         # buf
      li a2, 14          # count
      ecall

      # exit(0)
      li a7, 93          # SYS_exit
      li a0, 0           # status
      ecall

  .section .rodata
  msg:
      .ascii "Hello World!\n\0"

  编译：
  riscv64-linux-musl-as -o minimal.o minimal.S
  riscv64-linux-musl-ld -o minimal minimal.o

  ═══════════════════════════════════════════════════════════════
  任务4.7：系统调用实现示例
  ═══════════════════════════════════════════════════════════════

  /* kernel/syscalls/write.c */

  #include <minix/syscall.h>
  #include <minix/fs.h>
  #include <minix/errno.h>

  // writev - musl 的 printf 会用到这个
  long sys_writev(int fd, const struct iovec *iov, int iovcnt)
  {
      struct file *file;
      ssize_t total = 0;

      if (fd < 0 || iovcnt < 0)
          return -EINVAL;

      file = fget(fd);
      if (!file)
          return -EBADF;

      for (int i = 0; i < iovcnt; i++) {
          if (iov[i].iov_len == 0)
              continue;

          ssize_t ret = vfs_write(file, iov[i].iov_base,
                                  iov[i].iov_len, &file->f_pos);
          if (ret < 0) {
              fput(file);
              return ret;
          }
          total += ret;
      }

      fput(file);
      return total;
  }

  /* kernel/syscalls/mmap.c */

  // mmap - musl 的 malloc 大块分配用这个
  long sys_mmap(unsigned long addr, size_t len, int prot,
                int flags, int fd, off_t offset)
  {
      struct mm_struct *mm = current->mm;

      // 简化实现：只支持匿名映射
      if (!(flags & MAP_ANONYMOUS)) {
          // 文件映射需要更复杂的实现
          if (fd >= 0)
              return -ENODEV;  // 暂不支持
      }

      // 分配虚拟地址空间
      unsigned long ret = do_mmap(mm, addr, len, prot, flags, fd, offset);

      return ret;
  }

  ═══════════════════════════════════════════════════════════════
  任务4.8：测试验收
  ═══════════════════════════════════════════════════════════════

  阶段4分步验收：

  Step 1: 最小汇编程序运行
  - ⬜ minimal.S 输出 "Hello World!"
  - ⬜ sys_write + sys_exit 正常工作

  Step 2: musl 静态程序启动
  - ⬜ 内核正确设置用户栈（argc/argv/envp/auxv）
  - ⬜ _start → __libc_start_main → main 链正常
  - ⬜ printf("Hello") 正常输出

  Step 3: 完整 libc 功能
  - ⬜ malloc/free 正常（brk + mmap）
  - ⬜ 文件操作（open/read/write/close）
  - ⬜ fork/exec 正常
  - ⬜ 信号处理正常

  测试程序：

  /* userspace/test_musl.c */
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/wait.h>
  #include <errno.h>

  int main(int argc, char **argv)
  {
      printf("=== musl libc Test Suite ===\n\n");

      // Test 1: 基本 I/O
      printf("[TEST 1] printf works!\n");

      // Test 2: malloc
      printf("[TEST 2] malloc: ");
      char *buf = malloc(256);
      if (buf) {
          strcpy(buf, "Dynamic memory works!");
          printf("%s\n", buf);
          free(buf);
      } else {
          printf("FAILED\n");
          return 1;
      }

      // Test 3: 文件操作
      printf("[TEST 3] File I/O: ");
      int fd = open("/tmp/test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd >= 0) {
          write(fd, "test\n", 5);
          close(fd);
          printf("OK\n");
      } else {
          printf("open failed: %s\n", strerror(errno));
      }

      // Test 4: fork
      printf("[TEST 4] fork: ");
      pid_t pid = fork();
      if (pid < 0) {
          printf("fork failed: %s\n", strerror(errno));
      } else if (pid == 0) {
          // 子进程
          printf("child (pid=%d)\n", getpid());
          exit(42);
      } else {
          // 父进程
          int status;
          waitpid(pid, &status, 0);
          printf("parent, child exited with %d\n", WEXITSTATUS(status));
      }

      // Test 5: 环境变量
      printf("[TEST 5] Environment: ");
      char *path = getenv("PATH");
      printf("PATH=%s\n", path ? path : "(null)");

      printf("\n=== All tests completed ===\n");
      return 0;
  }

  编译和运行：
  riscv64-linux-musl-gcc -static -o test_musl test_musl.c
  # 将 test_musl 放入 ramfs 或通过其他方式加载到内核
  # 在 shell 中执行：exec /test_musl

  阶段4验收标准（musl）：
  - ⬜ musl 交叉编译工具链可用
  - ⬜ 最小汇编程序正常运行
  - ⬜ musl 静态链接程序正常启动
  - ⬜ printf/scanf 等 stdio 函数工作
  - ⬜ malloc/free 正常工作（brk + mmap）
  - ⬜ 能编译和运行完整的用户程序

  ---
  阶段5：文件系统完善（2-3个月）

  ═══════════════════════════════════════════════════════════════
  任务5.1：完善VFS层（2周）
  ═══════════════════════════════════════════════════════════════

  当前状态：基本VFS框架已存在，需要增强以支持完整POSIX语义

  核心数据结构升级：

  /* include/minix/fs.h */

  struct file {
      struct dentry *f_dentry;        // 目录项
      struct file_operations *f_op;   // 文件操作
      atomic_t f_count;               // 引用计数
      unsigned int f_flags;           // 打开标志 O_RDONLY等
      fmode_t f_mode;                 // 访问模式
      loff_t f_pos;                   // 当前位置
      struct fown_struct f_owner;     // 异步I/O所有者
      void *private_data;             // 文件系统私有数据
      spinlock_t f_lock;              // 保护f_pos
  };

  struct dentry {
      atomic_t d_count;               // 引用计数
      unsigned int d_flags;           // dentry标志
      struct inode *d_inode;          // 关联的inode
      struct dentry *d_parent;        // 父目录
      struct qstr d_name;             // 文件名
      struct list_head d_lru;         // LRU链表
      struct list_head d_child;       // 父目录的子项链表
      struct list_head d_subdirs;     // 子目录链表
      struct list_head d_alias;       // inode的别名链表
      struct dentry_operations *d_op; // 操作函数
      struct super_block *d_sb;       // 超级块
      void *d_fsdata;                 // 文件系统私有数据
  };

  struct inode {
      umode_t i_mode;                 // 文件类型和权限
      uid_t i_uid;                    // 所有者UID
      gid_t i_gid;                    // 所有者GID
      unsigned int i_flags;           // 文件系统标志
      unsigned long i_ino;            // inode号
      dev_t i_rdev;                   // 设备号（设备文件）
      loff_t i_size;                  // 文件大小
      struct timespec i_atime;        // 访问时间
      struct timespec i_mtime;        // 修改时间
      struct timespec i_ctime;        // 状态改变时间
      unsigned int i_nlink;           // 硬链接数
      blkcnt_t i_blocks;              // 占用块数
      struct inode_operations *i_op;  // inode操作
      struct file_operations *i_fop;  // 默认文件操作
      struct super_block *i_sb;       // 超级块
      struct address_space *i_mapping;// 页缓存映射
      void *i_private;                // 文件系统私有数据
  };

  struct super_block {
      dev_t s_dev;                    // 设备号
      unsigned long s_blocksize;      // 块大小
      unsigned char s_blocksize_bits; // 块大小位数
      loff_t s_maxbytes;              // 最大文件大小
      struct file_system_type *s_type;// 文件系统类型
      struct super_operations *s_op;  // 超级块操作
      struct dentry *s_root;          // 根目录dentry
      int s_count;                    // 引用计数
      void *s_fs_info;                // 文件系统私有数据
      char s_id[32];                  // 文件系统名称
  };

  操作函数接口：

  struct file_operations {
      loff_t (*llseek)(struct file *, loff_t, int);
      ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
      ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
      int (*readdir)(struct file *, struct dir_context *);
      int (*open)(struct inode *, struct file *);
      int (*release)(struct inode *, struct file *);
      int (*fsync)(struct file *, loff_t, loff_t, int);
      int (*mmap)(struct file *, struct vm_area_struct *);
      long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
  };

  struct inode_operations {
      struct dentry *(*lookup)(struct inode *, struct dentry *, unsigned int);
      int (*create)(struct inode *, struct dentry *, umode_t, bool);
      int (*link)(struct dentry *, struct inode *, struct dentry *);
      int (*unlink)(struct inode *, struct dentry *);
      int (*symlink)(struct inode *, struct dentry *, const char *);
      int (*mkdir)(struct inode *, struct dentry *, umode_t);
      int (*rmdir)(struct inode *, struct dentry *);
      int (*rename)(struct inode *, struct dentry *, struct inode *, struct dentry *);
      int (*setattr)(struct dentry *, struct iattr *);
      int (*getattr)(const struct path *, struct kstat *, unsigned int, unsigned int);
  };

  路径解析实现：

  /* fs/namei.c */

  // 路径解析核心函数
  int path_lookup(const char *name, unsigned int flags, struct nameidata *nd)
  {
      int err;

      // 1. 确定起点（绝对路径用根目录，相对路径用cwd）
      if (*name == '/') {
          nd->path.dentry = current->fs->root;
          nd->path.mnt = current->fs->rootmnt;
      } else {
          nd->path.dentry = current->fs->pwd;
          nd->path.mnt = current->fs->pwdmnt;
      }

      // 2. 逐级解析路径组件
      while (*name) {
          const char *next;
          int len = get_next_component(name, &next);

          if (len == 1 && name[0] == '.') {
              // 当前目录，跳过
          } else if (len == 2 && name[0] == '.' && name[1] == '.') {
              // 父目录
              err = follow_dotdot(nd);
              if (err) return err;
          } else {
              // 普通组件，查找
              err = do_lookup(nd, name, len);
              if (err) return err;

              // 检查是否是挂载点
              if (d_mountpoint(nd->path.dentry))
                  follow_mount(&nd->path);

              // 检查是否是符号链接
              if (nd->path.dentry->d_inode->i_op->follow_link)
                  // 处理符号链接...
          }

          name = next;
      }

      return 0;
  }

  // Dentry缓存（dcache）
  #define DCACHE_HASH_SIZE 256
  static struct hlist_head dcache_hashtable[DCACHE_HASH_SIZE];
  static spinlock_t dcache_lock;

  struct dentry *d_lookup(struct dentry *parent, const struct qstr *name)
  {
      unsigned int hash = full_name_hash(parent, name->name, name->len);
      struct hlist_head *head = &dcache_hashtable[hash % DCACHE_HASH_SIZE];
      struct dentry *dentry;

      spin_lock(&dcache_lock);
      hlist_for_each_entry(dentry, head, d_hash) {
          if (dentry->d_parent == parent &&
              dentry->d_name.len == name->len &&
              memcmp(dentry->d_name.name, name->name, name->len) == 0) {
              dget(dentry);  // 增加引用计数
              spin_unlock(&dcache_lock);
              return dentry;
          }
      }
      spin_unlock(&dcache_lock);
      return NULL;
  }

  ═══════════════════════════════════════════════════════════════
  任务5.2：实现块设备层（2周）
  ═══════════════════════════════════════════════════════════════

  /* include/minix/blkdev.h */

  struct block_device {
      dev_t bd_dev;                   // 设备号
      struct inode *bd_inode;         // 块设备inode
      struct super_block *bd_super;   // 如果已挂载
      int bd_openers;                 // 打开计数
      struct gendisk *bd_disk;        // 关联的磁盘
      struct request_queue *bd_queue; // 请求队列
      unsigned long bd_block_size;    // 块大小
      loff_t bd_nr_sectors;           // 扇区数
  };

  struct gendisk {
      int major;                      // 主设备号
      int first_minor;                // 第一个次设备号
      int minors;                     // 次设备号数量
      char disk_name[32];             // 磁盘名称
      struct block_device_operations *fops;
      struct request_queue *queue;
      void *private_data;
      sector_t capacity;              // 容量（扇区数）
  };

  struct block_device_operations {
      int (*open)(struct block_device *, fmode_t);
      void (*release)(struct gendisk *, fmode_t);
      int (*ioctl)(struct block_device *, fmode_t, unsigned, unsigned long);
      int (*rw_page)(struct block_device *, sector_t, struct page *, bool);
  };

  // 块I/O请求
  struct bio {
      struct block_device *bi_bdev;   // 目标设备
      sector_t bi_sector;             // 起始扇区
      unsigned int bi_size;           // 数据大小
      unsigned int bi_rw;             // 读/写标志
      struct bio_vec *bi_io_vec;      // 数据向量
      unsigned short bi_vcnt;         // 向量数量
      void (*bi_end_io)(struct bio *);// 完成回调
      void *bi_private;
  };

  struct bio_vec {
      struct page *bv_page;           // 数据页
      unsigned int bv_len;            // 长度
      unsigned int bv_offset;         // 页内偏移
  };

  块I/O实现：

  /* drivers/block/blkdev.c */

  // 提交块I/O请求
  void submit_bio(struct bio *bio)
  {
      struct block_device *bdev = bio->bi_bdev;
      struct request_queue *q = bdev->bd_queue;

      // 添加到请求队列
      spin_lock(&q->queue_lock);
      list_add_tail(&bio->bi_list, &q->bio_list);
      spin_unlock(&q->queue_lock);

      // 触发请求处理
      q->request_fn(q);
  }

  // 同步读取块
  int blkdev_read_block(struct block_device *bdev, sector_t sector,
                        void *buf, size_t size)
  {
      struct bio bio;
      struct bio_vec bvec;
      struct page *page;

      page = alloc_page(GFP_KERNEL);
      if (!page) return -ENOMEM;

      // 设置bio
      bio.bi_bdev = bdev;
      bio.bi_sector = sector;
      bio.bi_size = size;
      bio.bi_rw = READ;
      bio.bi_vcnt = 1;
      bio.bi_io_vec = &bvec;

      bvec.bv_page = page;
      bvec.bv_len = size;
      bvec.bv_offset = 0;

      // 提交并等待
      submit_bio_wait(&bio);

      // 复制数据
      memcpy(buf, page_address(page), size);
      free_page(page);

      return 0;
  }

  ═══════════════════════════════════════════════════════════════
  任务5.3：实现缓冲区缓存（Buffer Cache）（1周）
  ═══════════════════════════════════════════════════════════════

  /* fs/buffer.c */

  struct buffer_head {
      unsigned long b_state;          // 缓冲区状态
      struct buffer_head *b_this_page;// 同一页的下一个
      struct page *b_page;            // 所属页
      sector_t b_blocknr;             // 块号
      size_t b_size;                  // 块大小
      char *b_data;                   // 数据指针
      struct block_device *b_bdev;    // 块设备
      atomic_t b_count;               // 引用计数
      spinlock_t b_lock;
  };

  // 缓冲区状态标志
  enum bh_state_bits {
      BH_Uptodate,    // 数据有效
      BH_Dirty,       // 需要写回
      BH_Lock,        // 正在I/O
      BH_Mapped,      // 已映射到磁盘
      BH_New,         // 新分配
  };

  // 获取块缓冲区
  struct buffer_head *__bread(struct block_device *bdev,
                               sector_t block, unsigned size)
  {
      struct buffer_head *bh;

      // 1. 先在缓存中查找
      bh = __find_get_block(bdev, block, size);
      if (bh && buffer_uptodate(bh))
          return bh;

      // 2. 缓存未命中，分配新缓冲区
      if (!bh) {
          bh = __getblk(bdev, block, size);
          if (!bh) return NULL;
      }

      // 3. 从磁盘读取
      lock_buffer(bh);
      if (!buffer_uptodate(bh)) {
          bh->b_end_io = end_buffer_read_sync;
          submit_bh(READ, bh);
          wait_on_buffer(bh);
      }

      return bh;
  }

  // 标记脏并延迟写回
  void mark_buffer_dirty(struct buffer_head *bh)
  {
      if (!test_set_buffer_dirty(bh)) {
          // 添加到脏链表，等待pdflush回写
          spin_lock(&buffer_dirty_lock);
          list_add_tail(&bh->b_dirty_list, &buffer_dirty_list);
          spin_unlock(&buffer_dirty_lock);
      }
  }

  ═══════════════════════════════════════════════════════════════
  任务5.4：实现页缓存（Page Cache）（1周）
  ═══════════════════════════════════════════════════════════════

  /* mm/filemap.c */

  // address_space 管理文件的页缓存
  struct address_space {
      struct inode *host;             // 所属inode
      struct radix_tree_root page_tree;// 页缓存树
      spinlock_t tree_lock;
      unsigned long nrpages;          // 缓存页数
      const struct address_space_operations *a_ops;
  };

  struct address_space_operations {
      int (*writepage)(struct page *, struct writeback_control *);
      int (*readpage)(struct file *, struct page *);
      int (*write_begin)(struct file *, struct address_space *,
                         loff_t, unsigned, unsigned, struct page **, void **);
      int (*write_end)(struct file *, struct address_space *,
                       loff_t, unsigned, unsigned, struct page *, void *);
  };

  // 查找或创建页缓存页
  struct page *find_or_create_page(struct address_space *mapping,
                                    pgoff_t index, gfp_t gfp_mask)
  {
      struct page *page;

      // 1. 先查找
      spin_lock(&mapping->tree_lock);
      page = radix_tree_lookup(&mapping->page_tree, index);
      if (page) {
          get_page(page);
          spin_unlock(&mapping->tree_lock);
          return page;
      }
      spin_unlock(&mapping->tree_lock);

      // 2. 分配新页
      page = alloc_page(gfp_mask);
      if (!page) return NULL;

      // 3. 插入缓存
      spin_lock(&mapping->tree_lock);
      if (radix_tree_insert(&mapping->page_tree, index, page) == 0) {
          page->mapping = mapping;
          page->index = index;
          mapping->nrpages++;
      } else {
          // 竞争失败，释放页
          free_page(page);
          page = radix_tree_lookup(&mapping->page_tree, index);
          get_page(page);
      }
      spin_unlock(&mapping->tree_lock);

      return page;
  }

  // 通用文件读取（使用页缓存）
  ssize_t generic_file_read(struct file *filp, char __user *buf,
                            size_t count, loff_t *ppos)
  {
      struct address_space *mapping = filp->f_mapping;
      struct inode *inode = mapping->host;
      pgoff_t index;
      unsigned long offset;
      ssize_t ret = 0;

      while (count > 0) {
          index = *ppos >> PAGE_SHIFT;
          offset = *ppos & ~PAGE_MASK;

          struct page *page = find_or_create_page(mapping, index, GFP_KERNEL);
          if (!page) return -ENOMEM;

          // 如果页不是最新的，从磁盘读取
          if (!PageUptodate(page)) {
              int err = mapping->a_ops->readpage(filp, page);
              if (err) {
                  put_page(page);
                  return err;
              }
          }

          // 复制到用户空间
          size_t bytes = min(count, PAGE_SIZE - offset);
          if (copy_to_user(buf, page_address(page) + offset, bytes)) {
              put_page(page);
              return -EFAULT;
          }

          put_page(page);
          buf += bytes;
          count -= bytes;
          *ppos += bytes;
          ret += bytes;
      }

      return ret;
  }

  ═══════════════════════════════════════════════════════════════
  任务5.5：实现 ext2 文件系统（4周）
  ═══════════════════════════════════════════════════════════════

  ext2 磁盘布局：

  ┌──────────┬───────────┬───────────┬───────────┬───────────┬─────────┐
  │ Boot     │ Super     │ Group     │ Block     │ Inode     │ Data    │
  │ Block    │ Block     │ Desc      │ Bitmap    │ Bitmap    │ Blocks  │
  │ (1024B)  │           │ Table     │           │           │         │
  └──────────┴───────────┴───────────┴───────────┴───────────┴─────────┘
       0          1           2           3           4         5...

  /* fs/ext2/ext2.h */

  // 超级块（磁盘上）
  struct ext2_super_block {
      __le32 s_inodes_count;          // inode总数
      __le32 s_blocks_count;          // 块总数
      __le32 s_r_blocks_count;        // 保留块数
      __le32 s_free_blocks_count;     // 空闲块数
      __le32 s_free_inodes_count;     // 空闲inode数
      __le32 s_first_data_block;      // 第一个数据块
      __le32 s_log_block_size;        // 块大小 = 1024 << s_log_block_size
      __le32 s_blocks_per_group;      // 每组块数
      __le32 s_inodes_per_group;      // 每组inode数
      __le32 s_mtime;                 // 最后挂载时间
      __le32 s_wtime;                 // 最后写入时间
      __le16 s_mnt_count;             // 挂载次数
      __le16 s_max_mnt_count;         // 最大挂载次数
      __le16 s_magic;                 // 魔数 0xEF53
      __le16 s_state;                 // 文件系统状态
      __le16 s_errors;                // 错误处理方式
      __le32 s_first_ino;             // 第一个非保留inode
      __le16 s_inode_size;            // inode大小
      // ... 更多字段
  };

  // 块组描述符
  struct ext2_group_desc {
      __le32 bg_block_bitmap;         // 块位图块号
      __le32 bg_inode_bitmap;         // inode位图块号
      __le32 bg_inode_table;          // inode表起始块号
      __le16 bg_free_blocks_count;    // 空闲块数
      __le16 bg_free_inodes_count;    // 空闲inode数
      __le16 bg_used_dirs_count;      // 目录数
      __le16 bg_pad;
      __le32 bg_reserved[3];
  };

  // inode（磁盘上）
  struct ext2_inode {
      __le16 i_mode;                  // 文件类型和权限
      __le16 i_uid;                   // 所有者UID
      __le32 i_size;                  // 文件大小
      __le32 i_atime;                 // 访问时间
      __le32 i_ctime;                 // 创建时间
      __le32 i_mtime;                 // 修改时间
      __le32 i_dtime;                 // 删除时间
      __le16 i_gid;                   // 所有者GID
      __le16 i_links_count;           // 硬链接数
      __le32 i_blocks;                // 占用块数（512字节单位）
      __le32 i_flags;                 // 文件标志
      __le32 i_block[15];             // 块指针
      // i_block[0-11]  : 直接块
      // i_block[12]    : 一级间接块
      // i_block[13]    : 二级间接块
      // i_block[14]    : 三级间接块
      // ... 更多字段
  };

  // 目录项
  struct ext2_dir_entry_2 {
      __le32 inode;                   // inode号
      __le16 rec_len;                 // 目录项长度
      __u8   name_len;                // 文件名长度
      __u8   file_type;               // 文件类型
      char   name[];                  // 文件名（可变长）
  };

  ext2 核心操作实现：

  /* fs/ext2/inode.c */

  // 读取磁盘inode
  struct inode *ext2_iget(struct super_block *sb, unsigned long ino)
  {
      struct ext2_sb_info *sbi = sb->s_fs_info;
      struct buffer_head *bh;
      struct ext2_inode *raw_inode;
      struct inode *inode;
      unsigned long block_group, block, offset;

      // 计算inode所在位置
      block_group = (ino - 1) / sbi->s_inodes_per_group;
      offset = (ino - 1) % sbi->s_inodes_per_group;

      struct ext2_group_desc *gdp = ext2_get_group_desc(sb, block_group);
      block = gdp->bg_inode_table +
              (offset * sbi->s_inode_size) / sb->s_blocksize;

      // 读取inode块
      bh = sb_bread(sb, block);
      if (!bh) return ERR_PTR(-EIO);

      raw_inode = (struct ext2_inode *)
                  (bh->b_data + (offset % inodes_per_block) * sbi->s_inode_size);

      // 分配VFS inode并填充
      inode = new_inode(sb);
      inode->i_ino = ino;
      inode->i_mode = le16_to_cpu(raw_inode->i_mode);
      inode->i_uid = le16_to_cpu(raw_inode->i_uid);
      inode->i_gid = le16_to_cpu(raw_inode->i_gid);
      inode->i_size = le32_to_cpu(raw_inode->i_size);
      inode->i_nlink = le16_to_cpu(raw_inode->i_links_count);

      // 设置操作函数
      if (S_ISREG(inode->i_mode)) {
          inode->i_op = &ext2_file_inode_operations;
          inode->i_fop = &ext2_file_operations;
      } else if (S_ISDIR(inode->i_mode)) {
          inode->i_op = &ext2_dir_inode_operations;
          inode->i_fop = &ext2_dir_operations;
      } else if (S_ISLNK(inode->i_mode)) {
          inode->i_op = &ext2_symlink_inode_operations;
      }

      brelse(bh);
      return inode;
  }

  // 获取文件块号（处理间接块）
  static int ext2_get_block(struct inode *inode, sector_t iblock,
                            struct buffer_head *bh_result, int create)
  {
      struct ext2_inode_info *ei = EXT2_I(inode);
      int ptrs = EXT2_ADDR_PER_BLOCK(inode->i_sb);
      int ptrs_bits = EXT2_ADDR_PER_BLOCK_BITS(inode->i_sb);
      const int direct_blocks = EXT2_NDIR_BLOCKS;
      const int indirect_blocks = ptrs;
      const int double_blocks = ptrs * ptrs;

      if (iblock < direct_blocks) {
          // 直接块
          map_bh(bh_result, inode->i_sb, ei->i_data[iblock]);
      } else if (iblock < direct_blocks + indirect_blocks) {
          // 一级间接块
          iblock -= direct_blocks;
          ext2_get_branch(inode, 1, &ei->i_data[EXT2_IND_BLOCK],
                          iblock, bh_result, create);
      } else if (iblock < direct_blocks + indirect_blocks + double_blocks) {
          // 二级间接块
          iblock -= direct_blocks + indirect_blocks;
          ext2_get_branch(inode, 2, &ei->i_data[EXT2_DIND_BLOCK],
                          iblock, bh_result, create);
      } else {
          // 三级间接块
          iblock -= direct_blocks + indirect_blocks + double_blocks;
          ext2_get_branch(inode, 3, &ei->i_data[EXT2_TIND_BLOCK],
                          iblock, bh_result, create);
      }

      return 0;
  }

  /* fs/ext2/dir.c */

  // 目录查找
  struct dentry *ext2_lookup(struct inode *dir, struct dentry *dentry,
                              unsigned int flags)
  {
      struct inode *inode = NULL;
      ino_t ino;

      // 在目录中查找文件名
      ino = ext2_inode_by_name(dir, &dentry->d_name);
      if (ino) {
          inode = ext2_iget(dir->i_sb, ino);
          if (IS_ERR(inode))
              return ERR_CAST(inode);
      }

      return d_splice_alias(inode, dentry);
  }

  // 在目录中查找inode号
  ino_t ext2_inode_by_name(struct inode *dir, const struct qstr *name)
  {
      struct ext2_dir_entry_2 *de;
      struct buffer_head *bh;
      int namelen = name->len;
      const char *fname = name->name;
      loff_t pos = 0;

      while (pos < dir->i_size) {
          bh = ext2_bread(dir, pos >> dir->i_sb->s_blocksize_bits);
          if (!bh) return 0;

          de = (struct ext2_dir_entry_2 *)bh->b_data;
          while ((char *)de < bh->b_data + bh->b_size) {
              if (de->inode && de->name_len == namelen &&
                  memcmp(de->name, fname, namelen) == 0) {
                  ino_t ino = le32_to_cpu(de->inode);
                  brelse(bh);
                  return ino;
              }
              de = (void *)de + le16_to_cpu(de->rec_len);
          }
          brelse(bh);
          pos += bh->b_size;
      }

      return 0;  // 未找到
  }

  ═══════════════════════════════════════════════════════════════
  任务5.6：实现 procfs（选做，1周）
  ═══════════════════════════════════════════════════════════════

  procfs 提供进程和系统信息的伪文件系统：

  /proc/
  ├── [pid]/
  │   ├── status      # 进程状态
  │   ├── cmdline     # 命令行
  │   ├── maps        # 内存映射
  │   ├── fd/         # 打开的文件描述符
  │   └── stat        # 进程统计
  ├── cpuinfo         # CPU信息
  ├── meminfo         # 内存信息
  ├── uptime          # 运行时间
  ├── version         # 内核版本
  └── mounts          # 挂载信息

  /* fs/proc/base.c */

  static ssize_t proc_pid_status_read(struct file *file, char __user *buf,
                                       size_t count, loff_t *ppos)
  {
      struct task_struct *task = get_proc_task(file->f_path.dentry->d_inode);
      char buffer[256];
      int len;

      len = snprintf(buffer, sizeof(buffer),
          "Name:\t%s\n"
          "State:\t%c\n"
          "Pid:\t%d\n"
          "PPid:\t%d\n"
          "Uid:\t%d\n"
          "Gid:\t%d\n",
          task->comm,
          task_state_char(task),
          task->pid,
          task->parent ? task->parent->pid : 0,
          task->uid,
          task->gid);

      return simple_read_from_buffer(buf, count, ppos, buffer, len);
  }

  阶段5验收标准：
  - ⬜ VFS 层支持完整的路径解析和 dcache
  - ⬜ 块设备层能正确处理 I/O 请求
  - ⬜ 缓冲区缓存减少磁盘访问
  - ⬜ 页缓存加速文件读写
  - ⬜ ext2 文件系统能正常挂载和读写
  - ⬜ 能在 ext2 分区上创建/删除文件和目录

  ---
  阶段6：高级特性与测试（2-4个月）

  ═══════════════════════════════════════════════════════════════
  任务6.1：实现信号机制（3周）
  ═══════════════════════════════════════════════════════════════

  信号是 Unix 进程间通信的基础，musl 程序需要信号支持。

  信号列表（POSIX 必需）：

  #define SIGHUP      1   // 挂起
  #define SIGINT      2   // 中断 (Ctrl+C)
  #define SIGQUIT     3   // 退出 (Ctrl+\)
  #define SIGILL      4   // 非法指令
  #define SIGTRAP     5   // 断点
  #define SIGABRT     6   // abort()
  #define SIGBUS      7   // 总线错误
  #define SIGFPE      8   // 浮点异常
  #define SIGKILL     9   // 强制终止（不可捕获）
  #define SIGUSR1    10   // 用户定义1
  #define SIGSEGV    11   // 段错误
  #define SIGUSR2    12   // 用户定义2
  #define SIGPIPE    13   // 管道破裂
  #define SIGALRM    14   // 定时器
  #define SIGTERM    15   // 终止
  #define SIGCHLD    17   // 子进程状态改变
  #define SIGCONT    18   // 继续执行
  #define SIGSTOP    19   // 停止（不可捕获）
  #define SIGTSTP    20   // 终端停止 (Ctrl+Z)

  核心数据结构：

  /* include/minix/signal.h */

  typedef void (*sighandler_t)(int);
  typedef unsigned long sigset_t;

  struct sigaction {
      union {
          sighandler_t sa_handler;    // 简单处理函数
          void (*sa_sigaction)(int, siginfo_t *, void *);  // 带信息的处理
      };
      sigset_t sa_mask;               // 执行时阻塞的信号
      int sa_flags;                   // SA_SIGINFO, SA_RESTART 等
      void (*sa_restorer)(void);      // 信号返回 trampoline
  };

  struct sigpending {
      struct list_head list;          // 待处理信号链表
      sigset_t signal;                // 待处理信号位图
  };

  // 每个进程的信号状态
  struct signal_struct {
      atomic_t count;                 // 引用计数
      struct sigpending shared_pending;// 共享待处理信号
      struct k_sigaction action[64];  // 信号处理动作
  };

  // 内核态信号动作
  struct k_sigaction {
      struct sigaction sa;
  };

  信号发送实现：

  /* kernel/signal.c */

  // 发送信号给进程
  int send_signal(int sig, struct task_struct *t)
  {
      struct sigpending *pending;
      struct sigqueue *q;

      // 1. 检查信号是否被忽略
      if (sig_ignored(t, sig))
          return 0;

      // 2. 检查信号是否被阻塞（除了 SIGKILL/SIGSTOP）
      if (sigismember(&t->blocked, sig) && sig != SIGKILL && sig != SIGSTOP) {
          // 添加到待处理队列
          pending = &t->pending;
      } else {
          pending = &t->signal->shared_pending;
      }

      // 3. 添加到待处理信号集
      sigaddset(&pending->signal, sig);

      // 4. 如果进程在睡眠，唤醒它
      if (t->state == TASK_INTERRUPTIBLE)
          wake_up_process(t);

      // 5. 设置 TIF_SIGPENDING 标志
      set_tsk_thread_flag(t, TIF_SIGPENDING);

      return 0;
  }

  // sys_kill 实现
  SYSCALL_DEFINE2(kill, pid_t, pid, int, sig)
  {
      struct task_struct *p;

      if (sig < 0 || sig > 64)
          return -EINVAL;

      if (pid > 0) {
          // 发送给指定进程
          p = find_task_by_pid(pid);
          if (!p) return -ESRCH;
          return send_signal(sig, p);
      } else if (pid == 0) {
          // 发送给同进程组
          return kill_pgrp(current->pgrp, sig);
      } else if (pid == -1) {
          // 发送给所有进程
          return kill_all(sig);
      } else {
          // 发送给指定进程组
          return kill_pgrp(-pid, sig);
      }
  }

  信号处理实现：

  /* arch/riscv64/kernel/signal.c */

  // 信号栈帧（用户态栈上）
  struct rt_sigframe {
      struct siginfo info;            // 信号信息
      struct ucontext uc;             // 用户上下文
      // sigreturn trampoline 代码
  };

  struct ucontext {
      unsigned long uc_flags;
      struct ucontext *uc_link;
      stack_t uc_stack;               // 信号栈
      sigset_t uc_sigmask;            // 保存的信号掩码
      struct sigcontext uc_mcontext;  // 保存的寄存器
  };

  struct sigcontext {
      unsigned long sc_regs[32];      // 通用寄存器
      unsigned long sc_pc;            // 程序计数器
      // 浮点寄存器...
  };

  // 在返回用户态前检查待处理信号
  void do_signal(struct pt_regs *regs)
  {
      struct ksignal ksig;

      // 获取下一个待处理信号
      if (!get_signal(&ksig))
          return;

      // 处理信号
      handle_signal(&ksig, regs);
  }

  // 设置信号处理栈帧
  static int setup_rt_frame(struct ksignal *ksig, struct pt_regs *regs)
  {
      struct rt_sigframe __user *frame;
      unsigned long sp = regs->sp;

      // 1. 在用户栈上分配信号帧
      sp -= sizeof(*frame);
      sp &= ~15;  // 16字节对齐
      frame = (struct rt_sigframe __user *)sp;

      // 2. 保存当前上下文
      if (copy_siginfo_to_user(&frame->info, &ksig->info))
          return -EFAULT;

      // 保存寄存器到 uc_mcontext
      for (int i = 0; i < 32; i++)
          frame->uc.uc_mcontext.sc_regs[i] = regs->regs[i];
      frame->uc.uc_mcontext.sc_pc = regs->sepc;

      // 3. 设置 sigreturn trampoline
      // 用户态执行完信号处理函数后会调用 rt_sigreturn
      frame->uc.uc_mcontext.sc_regs[1] = (unsigned long)&frame->retcode;
      // retcode: li a7, __NR_rt_sigreturn; ecall

      // 4. 修改寄存器，跳转到信号处理函数
      regs->sepc = (unsigned long)ksig->ka.sa.sa_handler;
      regs->sp = (unsigned long)frame;
      regs->regs[10] = ksig->sig;  // a0 = 信号号

      if (ksig->ka.sa.sa_flags & SA_SIGINFO) {
          regs->regs[11] = (unsigned long)&frame->info;     // a1 = siginfo
          regs->regs[12] = (unsigned long)&frame->uc;       // a2 = ucontext
      }

      return 0;
  }

  // 信号返回系统调用
  SYSCALL_DEFINE0(rt_sigreturn)
  {
      struct pt_regs *regs = current_pt_regs();
      struct rt_sigframe __user *frame;

      frame = (struct rt_sigframe __user *)(regs->sp);

      // 恢复信号掩码
      sigset_t set;
      if (copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
          return -EFAULT;
      set_current_blocked(&set);

      // 恢复寄存器
      for (int i = 0; i < 32; i++)
          regs->regs[i] = frame->uc.uc_mcontext.sc_regs[i];
      regs->sepc = frame->uc.uc_mcontext.sc_pc;

      return regs->regs[10];  // 返回 a0
  }

  sigaction 系统调用：

  SYSCALL_DEFINE4(rt_sigaction, int, sig,
                  const struct sigaction __user *, act,
                  struct sigaction __user *, oact,
                  size_t, sigsetsize)
  {
      struct k_sigaction new_ka, old_ka;

      if (sig < 1 || sig > 64)
          return -EINVAL;

      // 不能改变 SIGKILL 和 SIGSTOP 的处理
      if (sig == SIGKILL || sig == SIGSTOP)
          return -EINVAL;

      // 保存旧动作
      if (oact) {
          old_ka = current->sighand->action[sig - 1];
          if (copy_to_user(oact, &old_ka.sa, sizeof(*oact)))
              return -EFAULT;
      }

      // 设置新动作
      if (act) {
          if (copy_from_user(&new_ka.sa, act, sizeof(*act)))
              return -EFAULT;
          current->sighand->action[sig - 1] = new_ka;
      }

      return 0;
  }

  ═══════════════════════════════════════════════════════════════
  任务6.2：实现管道（Pipe）（1周）
  ═══════════════════════════════════════════════════════════════

  管道是最基本的 IPC 机制。

  /* fs/pipe.c */

  #define PIPE_BUF_SIZE   4096    // POSIX 最小保证原子写入大小

  struct pipe_inode_info {
      struct mutex mutex;             // 保护管道
      wait_queue_head_t wait;         // 等待队列
      unsigned int head;              // 读位置
      unsigned int tail;              // 写位置
      unsigned int readers;           // 读端计数
      unsigned int writers;           // 写端计数
      struct page *bufs;              // 数据缓冲区
  };

  // 创建管道
  SYSCALL_DEFINE2(pipe2, int __user *, fildes, int, flags)
  {
      struct file *files[2];
      int fd[2];
      int err;

      // 1. 分配管道 inode
      struct inode *inode = get_pipe_inode();
      if (!inode) return -ENFILE;

      struct pipe_inode_info *pipe = inode->i_pipe;

      // 2. 创建读端文件
      files[0] = alloc_file(FMODE_READ);
      files[0]->f_op = &read_pipefifo_fops;
      files[0]->private_data = pipe;
      pipe->readers++;

      // 3. 创建写端文件
      files[1] = alloc_file(FMODE_WRITE);
      files[1]->f_op = &write_pipefifo_fops;
      files[1]->private_data = pipe;
      pipe->writers++;

      // 4. 分配文件描述符
      fd[0] = get_unused_fd_flags(flags);
      fd[1] = get_unused_fd_flags(flags);

      fd_install(fd[0], files[0]);
      fd_install(fd[1], files[1]);

      // 5. 返回给用户
      if (copy_to_user(fildes, fd, sizeof(fd)))
          return -EFAULT;

      return 0;
  }

  // 管道读取
  static ssize_t pipe_read(struct file *filp, char __user *buf,
                           size_t count, loff_t *ppos)
  {
      struct pipe_inode_info *pipe = filp->private_data;
      ssize_t ret = 0;

      mutex_lock(&pipe->mutex);

      while (count > 0) {
          // 检查是否有数据
          unsigned int head = pipe->head;
          unsigned int tail = pipe->tail;

          if (head == tail) {
              // 管道为空
              if (pipe->writers == 0) {
                  // 写端已关闭，返回 EOF
                  break;
              }
              if (filp->f_flags & O_NONBLOCK) {
                  ret = ret ? ret : -EAGAIN;
                  break;
              }
              // 等待数据
              mutex_unlock(&pipe->mutex);
              wait_event_interruptible(pipe->wait, pipe->head != pipe->tail);
              mutex_lock(&pipe->mutex);
              continue;
          }

          // 读取数据
          size_t available = tail - head;
          size_t to_read = min(count, available);

          char *src = page_address(pipe->bufs) + (head % PIPE_BUF_SIZE);
          if (copy_to_user(buf, src, to_read)) {
              ret = -EFAULT;
              break;
          }

          pipe->head += to_read;
          buf += to_read;
          count -= to_read;
          ret += to_read;

          // 唤醒写者
          wake_up_interruptible(&pipe->wait);
      }

      mutex_unlock(&pipe->mutex);
      return ret;
  }

  // 管道写入
  static ssize_t pipe_write(struct file *filp, const char __user *buf,
                            size_t count, loff_t *ppos)
  {
      struct pipe_inode_info *pipe = filp->private_data;
      ssize_t ret = 0;

      mutex_lock(&pipe->mutex);

      // 检查读端是否已关闭
      if (pipe->readers == 0) {
          send_signal(SIGPIPE, current);
          ret = -EPIPE;
          goto out;
      }

      while (count > 0) {
          unsigned int head = pipe->head;
          unsigned int tail = pipe->tail;
          unsigned int space = PIPE_BUF_SIZE - (tail - head);

          if (space == 0) {
              // 管道满
              if (filp->f_flags & O_NONBLOCK) {
                  ret = ret ? ret : -EAGAIN;
                  break;
              }
              // 等待空间
              mutex_unlock(&pipe->mutex);
              wait_event_interruptible(pipe->wait,
                  PIPE_BUF_SIZE - (pipe->tail - pipe->head) > 0);
              mutex_lock(&pipe->mutex);
              continue;
          }

          // 写入数据
          size_t to_write = min(count, space);
          char *dst = page_address(pipe->bufs) + (tail % PIPE_BUF_SIZE);

          if (copy_from_user(dst, buf, to_write)) {
              ret = -EFAULT;
              break;
          }

          pipe->tail += to_write;
          buf += to_write;
          count -= to_write;
          ret += to_write;

          // 唤醒读者
          wake_up_interruptible(&pipe->wait);
      }

  out:
      mutex_unlock(&pipe->mutex);
      return ret;
  }

  ═══════════════════════════════════════════════════════════════
  任务6.3：实现设备驱动框架（2周）
  ═══════════════════════════════════════════════════════════════

  字符设备框架：

  /* include/minix/cdev.h */

  struct cdev {
      struct kobject kobj;
      struct module *owner;
      const struct file_operations *ops;
      struct list_head list;
      dev_t dev;                      // 设备号
      unsigned int count;             // 次设备号数量
  };

  // 字符设备注册表
  #define MAX_CHRDEV  256
  static struct char_device_struct {
      struct char_device_struct *next;
      unsigned int major;
      unsigned int baseminor;
      int minorct;
      char name[64];
      struct cdev *cdev;
  } *chrdevs[MAX_CHRDEV];

  // 注册字符设备
  int register_chrdev_region(dev_t from, unsigned count, const char *name)
  {
      unsigned int major = MAJOR(from);
      unsigned int minor = MINOR(from);

      struct char_device_struct *cd = kmalloc(sizeof(*cd), GFP_KERNEL);
      cd->major = major;
      cd->baseminor = minor;
      cd->minorct = count;
      strncpy(cd->name, name, sizeof(cd->name));

      // 添加到哈希表
      cd->next = chrdevs[major % MAX_CHRDEV];
      chrdevs[major % MAX_CHRDEV] = cd;

      return 0;
  }

  // 初始化 cdev
  void cdev_init(struct cdev *cdev, const struct file_operations *fops)
  {
      memset(cdev, 0, sizeof(*cdev));
      INIT_LIST_HEAD(&cdev->list);
      cdev->ops = fops;
  }

  // 添加 cdev
  int cdev_add(struct cdev *p, dev_t dev, unsigned count)
  {
      p->dev = dev;
      p->count = count;

      // 关联到注册表
      unsigned int major = MAJOR(dev);
      struct char_device_struct *cd = chrdevs[major % MAX_CHRDEV];
      while (cd && cd->major != major)
          cd = cd->next;

      if (cd)
          cd->cdev = p;

      return 0;
  }

  // 打开字符设备
  static int chrdev_open(struct inode *inode, struct file *filp)
  {
      struct cdev *p;
      dev_t dev = inode->i_rdev;

      // 查找 cdev
      p = lookup_cdev(dev);
      if (!p) return -ENXIO;

      // 设置文件操作
      filp->f_op = p->ops;

      // 调用设备的 open
      if (filp->f_op->open)
          return filp->f_op->open(inode, filp);

      return 0;
  }

  示例：TTY 设备驱动

  /* drivers/char/tty.c */

  static struct tty_struct *ttys[MAX_TTYS];

  static int tty_open(struct inode *inode, struct file *filp)
  {
      int minor = MINOR(inode->i_rdev);
      struct tty_struct *tty = ttys[minor];

      if (!tty)
          return -ENODEV;

      filp->private_data = tty;
      tty->count++;

      return 0;
  }

  static ssize_t tty_read(struct file *filp, char __user *buf,
                          size_t count, loff_t *ppos)
  {
      struct tty_struct *tty = filp->private_data;
      return tty->ldisc->ops->read(tty, filp, buf, count);
  }

  static ssize_t tty_write(struct file *filp, const char __user *buf,
                           size_t count, loff_t *ppos)
  {
      struct tty_struct *tty = filp->private_data;
      return tty->ldisc->ops->write(tty, filp, buf, count);
  }

  static long tty_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
  {
      struct tty_struct *tty = filp->private_data;

      switch (cmd) {
      case TCGETS:
          return copy_to_user((void *)arg, &tty->termios, sizeof(tty->termios));
      case TCSETS:
          return copy_from_user(&tty->termios, (void *)arg, sizeof(tty->termios));
      case TIOCGWINSZ:
          return copy_to_user((void *)arg, &tty->winsize, sizeof(tty->winsize));
      case TIOCSWINSZ:
          return copy_from_user(&tty->winsize, (void *)arg, sizeof(tty->winsize));
      default:
          return -ENOTTY;
      }
  }

  static const struct file_operations tty_fops = {
      .open = tty_open,
      .read = tty_read,
      .write = tty_write,
      .unlocked_ioctl = tty_ioctl,
      .release = tty_release,
  };

  ═══════════════════════════════════════════════════════════════
  任务6.4：实现 dup/dup2（1周）
  ═══════════════════════════════════════════════════════════════

  /* fs/fcntl.c */

  // 复制文件描述符
  SYSCALL_DEFINE1(dup, unsigned int, fildes)
  {
      struct file *file = fget(fildes);
      if (!file) return -EBADF;

      int newfd = get_unused_fd_flags(0);
      if (newfd < 0) {
          fput(file);
          return newfd;
      }

      fd_install(newfd, file);
      return newfd;
  }

  // 复制到指定描述符
  SYSCALL_DEFINE3(dup3, unsigned int, oldfd, unsigned int, newfd, int, flags)
  {
      struct file *file;

      if (oldfd == newfd)
          return -EINVAL;

      file = fget(oldfd);
      if (!file) return -EBADF;

      // 如果 newfd 已打开，先关闭
      struct file *old_file = fget(newfd);
      if (old_file) {
          fput(old_file);
          sys_close(newfd);
      }

      // 确保 newfd 可用
      int err = expand_files(current->files, newfd);
      if (err < 0) {
          fput(file);
          return err;
      }

      // 安装新文件描述符
      fd_install(newfd, file);

      if (flags & O_CLOEXEC)
          set_close_on_exec(newfd, 1);

      return newfd;
  }

  SYSCALL_DEFINE2(dup2, unsigned int, oldfd, unsigned int, newfd)
  {
      if (oldfd == newfd) {
          // POSIX: 如果相同，检查 oldfd 是否有效
          if (fget(oldfd))
              return newfd;
          return -EBADF;
      }
      return sys_dup3(oldfd, newfd, 0);
  }

  ═══════════════════════════════════════════════════════════════
  任务6.5：测试套件（持续）
  ═══════════════════════════════════════════════════════════════

  测试策略：

  1. 单元测试 - 每个子系统独立测试
  ┌─────────────────────────────────────────────────────────────┐
  │  测试文件           │  测试内容                             │
  │ ─────────────────── │ ───────────────────────────────────── │
  │  test_slab.c        │  kmalloc/kfree 正确性                 │
  │  test_buddy.c       │  页分配器功能                         │
  │  test_vfs.c         │  路径解析、文件操作                   │
  │  test_signal.c      │  信号发送/接收/处理                   │
  │  test_pipe.c        │  管道读写、阻塞行为                   │
  │  test_fork.c        │  进程创建、内存复制                   │
  └─────────────────────────────────────────────────────────────┘

  2. 系统测试 - 用户态程序测试

  /* userspace/test_posix.c */

  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <signal.h>
  #include <sys/wait.h>
  #include <sys/stat.h>

  #define TEST(name) printf("Testing %s... ", name)
  #define PASS() printf("PASS\n")
  #define FAIL(msg) do { printf("FAIL: %s\n", msg); exit(1); } while(0)

  // 测试 fork/exec/wait
  void test_process(void)
  {
      TEST("fork/wait");
      pid_t pid = fork();
      if (pid < 0) FAIL("fork failed");
      if (pid == 0) {
          exit(42);
      }
      int status;
      waitpid(pid, &status, 0);
      if (WEXITSTATUS(status) != 42) FAIL("wrong exit status");
      PASS();
  }

  // 测试管道
  void test_pipe(void)
  {
      TEST("pipe");
      int fd[2];
      if (pipe(fd) < 0) FAIL("pipe failed");

      const char *msg = "hello pipe";
      if (write(fd[1], msg, strlen(msg)) != strlen(msg)) FAIL("write failed");

      char buf[32];
      if (read(fd[0], buf, sizeof(buf)) != strlen(msg)) FAIL("read failed");
      if (strncmp(buf, msg, strlen(msg)) != 0) FAIL("data mismatch");

      close(fd[0]);
      close(fd[1]);
      PASS();
  }

  // 测试信号
  volatile int sig_received = 0;
  void sig_handler(int sig) { sig_received = sig; }

  void test_signal(void)
  {
      TEST("signal");
      signal(SIGUSR1, sig_handler);
      kill(getpid(), SIGUSR1);
      if (sig_received != SIGUSR1) FAIL("signal not received");
      PASS();
  }

  // 测试 dup
  void test_dup(void)
  {
      TEST("dup/dup2");
      int fd = open("/tmp/dup_test", O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0) FAIL("open failed");

      int fd2 = dup(fd);
      if (fd2 < 0) FAIL("dup failed");

      write(fd, "hello", 5);
      write(fd2, "world", 5);

      close(fd);
      close(fd2);

      // 验证内容
      fd = open("/tmp/dup_test", O_RDONLY);
      char buf[16];
      read(fd, buf, 10);
      if (strncmp(buf, "helloworld", 10) != 0) FAIL("content mismatch");
      close(fd);
      PASS();
  }

  int main(void)
  {
      printf("=== MinixRV64 POSIX Test Suite ===\n\n");

      test_process();
      test_pipe();
      test_signal();
      test_dup();

      printf("\n=== All tests passed! ===\n");
      return 0;
  }

  3. 兼容性测试 - 运行标准程序

  # 移植 busybox（静态编译）
  wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
  tar xf busybox-1.36.1.tar.bz2
  cd busybox-1.36.1

  make CROSS_COMPILE=riscv64-linux-musl- defconfig
  make CROSS_COMPILE=riscv64-linux-musl- LDFLAGS=--static -j$(nproc)

  # 测试基本命令
  ./busybox ls
  ./busybox cat /etc/passwd
  ./busybox echo hello

  4. 压力测试

  /* stress_test.c */
  // fork 炸弹防护测试
  // 大量文件描述符测试
  // 内存压力测试
  // 管道缓冲区溢出测试

  阶段6验收标准：
  - ⬜ 信号机制完整（SIGINT, SIGTERM, SIGKILL, SIGCHLD 等）
  - ⬜ 信号处理函数能正确执行和返回
  - ⬜ 管道读写正常，支持阻塞和非阻塞模式
  - ⬜ 设备驱动框架可用（字符设备）
  - ⬜ dup/dup2 正常工作
  - ⬜ 通过 POSIX 基本测试套件
  - ⬜ busybox 基本命令可运行

  ---
  🎯 最终验收标准

  必须达到的POSIX兼容性

  进程管理：
  - ⬜ fork(), vfork(), clone()
  - ⬜ exec() 系列
  - ⬜ wait(), waitpid()
  - ⬜ exit(), _exit()
  - ⬜ getpid(), getppid(), getuid()等

  文件系统：
  - ⬜ open(), close(), read(), write()
  - ⬜ lseek(), stat(), fstat()
  - ⬜ mkdir(), rmdir(), unlink()
  - ⬜ link(), symlink(), readlink()
  - ⬜ chmod(), chown()
  - ⬜ dup(), dup2()

  信号：
  - ⬜ kill(), signal(), sigaction()
  - ⬜ 至少支持SIGINT, SIGTERM, SIGKILL, SIGCHLD

  管道和IPC：
  - ⬜ pipe()
  - ⬜ 基本的共享内存（可选）

  内存管理：
  - ✅ brk(), sbrk() (基础框架已有)
  - ⬜ mmap(), munmap()

  能运行的标准程序

  # 基本shell
  /bin/sh

  # 核心工具（busybox）
  ls, cat, echo, mkdir, rm, cp, mv, grep, find

  # 编译器（如果移植成功）
  gcc, make

  # 简单应用
  hello world
  文件操作程序
  多进程程序

  ---
  📊 资源需求

  人力

  - 最少：1名有经验的系统程序员，全职1-2年
  - 理想：2-3人小团队，8-12个月

  技能要求

  - ✅ 深入理解操作系统原理
  - ✅ C语言高级特性
  - ✅ RISC-V汇编
  - ✅ Linux内核开发经验
  - ✅ 调试能力强

  硬件

  - 开发机：x86_64 Linux，16GB+ RAM
  - QEMU模拟器
  - 可选：真实RISC-V开发板（如HiFive Unmatched）

  参考资料

  - Linux内核源码
  - xv6（教学OS）
  - Minix 3源码
  - RISC-V ISA手册
  - POSIX标准文档

  ---
  🚧 风险和挑战

  技术难点

  1. 调试困难 - 内核bug难以定位
  2. 并发问题 - 竞态条件、死锁
  3. 性能优化 - 需要大量profiling
  4. 兼容性 - POSIX标准复杂

  缓解策略

  - 使用GDB远程调试QEMU
  - 广泛使用单元测试
  - 参考成熟内核实现
  - 循序渐进，不要急于求成

  ---
  🎓 学习资源

  书籍

  1. Operating Systems: Three Easy Pieces - 免费在线
  2. The Linux Programming Interface - Michael Kerrisk
  3. Understanding the Linux Kernel - Daniel Bovet
  4. RISC-V Reader - Patterson & Waterman

  在线课程

  1. MIT 6.S081 (xv6)
  2. CS162 Berkeley (Pintos)
  3. OSDev Wiki

  开源项目参考

  1. xv6-riscv - 简单教学OS
  2. seL4 - 形式化验证微内核
  3. Zephyr - RTOS
  4. Linux内核 - 生产级参考

  ---
  总结

  将MinixRV64提升到POSIX兼容是一个庞大但可行的项目。关键是：

  1. 循序渐进 - 按阶段完成，每个阶段都有可验证的成果
  2. 参考现有实现 - 不要重新发明轮子
  3. 持续测试 - 每个功能都要充分测试
  4. 文档完善 - 记录设计决策和已知问题

  如果严格按照这个路线图执行，12-24个月内可以达到基本的POSIX兼容，能够运行简单的Unix程序。这将是一个非常有价值的学习和实践项目！🚀

  ---
  📅 进度跟踪

  | 阶段 | 状态 | 开始日期 | 完成日期 | 备注 |
  |------|------|----------|----------|------|
  | 阶段1: 内存管理 | ✅ 完成 | - | 2024-12 | buddy/slab/MMU/vmalloc |
  | 阶段2: 进程管理 | ⬜ 待开始 | - | - | fork/exec/scheduler |
  | 阶段3: 系统调用 | ⬜ 待开始 | - | - | Linux RISC-V ABI |
  | 阶段4: C库移植 | ⬜ 待开始 | - | - | **musl libc** |
  | 阶段5: 文件系统 | ⬜ 待开始 | - | - | ext2/VFS/dcache/pagecache |
  | 阶段6: 高级特性 | ⬜ 待开始 | - | - | 信号/管道/设备驱动 |

  ---
  📝 变更记录

  | 日期 | 变更内容 |
  |------|----------|
  | 2024-12 | 阶段1完成：内存管理（buddy/slab/MMU/vmalloc） |
  | 2025-12 | 阶段4重新设计：从 newlib 改为 **musl libc** |
  | 2025-12 | 明确项目定位：Minix 微内核 + Linux syscall ABI |
  | 2025-12 | 项目命名：**MinixRV64 Donz Build** |
