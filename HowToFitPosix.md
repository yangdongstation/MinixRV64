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
  阶段2：进程管理实现（3-4个月）← 下一步

  当前问题

  - ❌ 没有真正的进程
  - ❌ 没有进程调度
  - ❌ 没有上下文切换

  任务2.1：设计进程数据结构（1周）

  /* include/minix/sched.h */

  struct mm_struct {
      pgd_t *pgd;                    // 页表
      unsigned long start_code;       // 代码段起始
      unsigned long end_code;
      unsigned long start_data;       // 数据段起始
      unsigned long end_data;
      unsigned long start_brk;        // 堆起始
      unsigned long brk;              // 堆当前位置
      unsigned long start_stack;      // 栈起始
      struct vm_area_struct *mmap;    // VMA链表
      int map_count;                  // VMA数量
  };

  struct vm_area_struct {
      unsigned long vm_start;         // 区域起始地址
      unsigned long vm_end;           // 区域结束地址
      unsigned long vm_flags;         // 权限标志
      struct vm_area_struct *vm_next;
      struct file *vm_file;           // 关联的文件（mmap）
  };

  struct task_struct {
      // 进程状态
      volatile long state;            // TASK_RUNNING, TASK_INTERRUPTIBLE等
      pid_t pid;
      pid_t ppid;                     // 父进程PID

      // 调度信息
      int prio;                       // 优先级
      unsigned long time_slice;       // 时间片
      unsigned long utime, stime;     // 用户态/内核态时间

      // 内存管理
      struct mm_struct *mm;

      // 文件系统
      struct fs_struct *fs;           // 文件系统信息
      struct files_struct *files;     // 打开文件表

      // 信号
      struct signal_struct *signal;
      sigset_t blocked;               // 阻塞的信号

      // 寄存器上下文
      struct pt_regs *regs;           // 保存的寄存器
      unsigned long kernel_sp;        // 内核栈指针

      // 进程关系
      struct task_struct *parent;
      struct list_head children;
      struct list_head sibling;

      // 等待队列
      wait_queue_head_t wait_child;

      // 退出状态
      int exit_code;

      // 调度链表
      struct list_head run_list;
  };

  任务2.2：实现进程创建（2周）

  步骤：

  1. 实现copy_process()
  /* kernel/fork.c */

  struct task_struct *copy_process(unsigned long clone_flags,
                                    struct pt_regs *regs)
  {
      struct task_struct *p;

      // 1. 分配task_struct
      p = alloc_task_struct();
      if (!p) return NULL;

      // 2. 复制父进程信息
      *p = *current;
      p->pid = alloc_pid();
      p->ppid = current->pid;
      p->state = TASK_RUNNING;

      // 3. 复制地址空间
      if (!(clone_flags & CLONE_VM)) {
          p->mm = copy_mm(current->mm);
      } else {
          p->mm = current->mm;  // 共享地址空间（线程）
      }

      // 4. 复制文件描述符表
      if (!(clone_flags & CLONE_FILES)) {
          p->files = copy_files(current->files);
      }

      // 5. 设置内核栈
      p->kernel_sp = (unsigned long)p + TASK_SIZE - sizeof(struct pt_regs);

      // 6. 复制寄存器
      struct pt_regs *child_regs = (struct pt_regs *)p->kernel_sp;
      *child_regs = *regs;
      child_regs->a0 = 0;  // fork()子进程返回0

      return p;
  }
  2. 实现do_fork()
  long do_fork(unsigned long clone_flags, struct pt_regs *regs)
  {
      struct task_struct *p;

      p = copy_process(clone_flags, regs);
      if (!p) return -ENOMEM;

      // 添加到就绪队列
      wake_up_new_task(p);

      return p->pid;
  }
  3. 实现sys_fork()
  SYSCALL_DEFINE0(fork)
  {
      struct pt_regs *regs = current_pt_regs();
      return do_fork(SIGCHLD, regs);
  }

  任务2.3：实现进程调度器（2周）

  采用简化版优先级调度（后续可升级为CFS）

  /* kernel/sched.c */

  struct rq {
      spinlock_t lock;
      struct list_head run_queue;     // 就绪队列
      unsigned long nr_running;
      struct task_struct *curr;
  };

  void scheduler_tick(void)
  {
      struct task_struct *curr = current;

      // 减少时间片
      if (curr->time_slice > 0)
          curr->time_slice--;

      // 时间片用完，需要调度
      if (curr->time_slice == 0) {
          curr->time_slice = DEFAULT_TIME_SLICE;
          set_tsk_need_resched(curr);
      }
  }

  void schedule(void)
  {
      struct task_struct *prev, *next;

      prev = current;

      // 选择下一个任务
      next = pick_next_task();

      if (prev != next) {
          // 上下文切换
          switch_to(prev, next);
      }
  }

  任务2.4：实现上下文切换（1周）

  /* arch/riscv64/kernel/switch.S */

  ENTRY(__switch_to)
      // 保存prev的寄存器
      sd sp,  0(a0)   // 保存sp
      sd s0,  8(a0)   // 保存s0-s11
      sd s1, 16(a0)
      // ... 保存其他callee-saved寄存器

      // 恢复next的寄存器
      ld sp,  0(a1)
      ld s0,  8(a1)
      ld s1, 16(a1)
      // ...

      // 切换页表
      ld a0, TASK_MM_PGD(a1)
      srli a0, a0, PAGE_SHIFT
      li a1, SATP_MODE_SV39
      or a0, a0, a1
      csrw satp, a0
      sfence.vma

      ret
  ENDPROC(__switch_to)

  任务2.5：实现exec系统调用（2周）

  /* fs/exec.c */

  int do_execve(const char *filename, char **argv, char **envp)
  {
      struct linux_binprm bprm;
      struct file *file;
      int ret;

      // 1. 打开可执行文件
      file = open_exec(filename);
      if (IS_ERR(file)) return PTR_ERR(file);

      // 2. 读取文件头
      ret = prepare_binprm(&bprm);
      if (ret < 0) goto out;

      // 3. 根据文件格式加载（ELF/script）
      ret = search_binary_handler(&bprm);
      if (ret < 0) goto out;

      // 4. 设置新的地址空间
      ret = setup_arg_pages(&bprm);

      // 5. 开始执行新程序
      start_thread(regs, bprm.entry, bprm.p);

  out:
      return ret;
  }

  任务2.6：实现ELF加载器（2周）

  /* fs/binfmt_elf.c */

  static int load_elf_binary(struct linux_binprm *bprm)
  {
      struct elfhdr *elf_ex = (struct elfhdr *)bprm->buf;
      struct elf_phdr *elf_ppnt;
      unsigned long load_addr = 0;
      int i;

      // 1. 验证ELF header
      if (memcmp(elf_ex->e_ident, ELFMAG, SELFMAG) != 0)
          return -ENOEXEC;

      // 2. 读取program headers
      elf_ppnt = load_elf_phdrs(elf_ex, bprm->file);

      // 3. 清空旧地址空间
      flush_old_exec(bprm);

      // 4. 加载各个段
      for (i = 0; i < elf_ex->e_phnum; i++) {
          if (elf_ppnt[i].p_type != PT_LOAD)
              continue;

          // 映射段到内存
          error = elf_map(bprm->file,
                         elf_ppnt[i].p_vaddr,
                         &elf_ppnt[i]);
      }

      // 5. 设置入口点
      bprm->entry = elf_ex->e_entry;

      return 0;
  }

  任务2.7：实现wait/exit（1周）

  /* kernel/exit.c */

  void do_exit(int code)
  {
      struct task_struct *tsk = current;

      // 1. 设置退出状态
      tsk->exit_code = code;

      // 2. 释放资源
      exit_mm(tsk);       // 释放地址空间
      exit_files(tsk);    // 关闭所有文件
      exit_fs(tsk);       // 释放文件系统信息

      // 3. 通知父进程
      tsk->state = TASK_ZOMBIE;
      wake_up_parent(tsk);

      // 4. 调度到其他进程
      schedule();

      // 永不返回
  }

  SYSCALL_DEFINE1(exit, int, error_code)
  {
      do_exit((error_code & 0xff) << 8);
  }

  SYSCALL_DEFINE4(wait4, pid_t, pid, int __user *, stat_addr,
                  int, options, struct rusage __user *, ru)
  {
      struct task_struct *p;
      int ret;

      // 等待子进程
      ret = wait_for_child(pid, stat_addr, options);

      return ret;
  }

  阶段2验收标准：
  - ⬜ 能创建新进程（fork）
  - ⬜ 能加载并执行ELF程序（exec）
  - ⬜ 进程能正常调度和切换
  - ⬜ 父子进程关系正确
  - ⬜ wait/exit正常工作

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

  任务5.1：完善VFS层（2周）

  /* fs/vfs.c - 增强版 */

  struct file {
      struct dentry *f_dentry;
      struct file_operations *f_op;
      atomic_t f_count;
      unsigned int f_flags;
      loff_t f_pos;
      void *private_data;
  };

  struct dentry {
      struct inode *d_inode;
      struct dentry *d_parent;
      struct qstr d_name;
      struct list_head d_subdirs;
      struct list_head d_child;
      struct dentry_operations *d_op;
  };

  struct inode {
      umode_t i_mode;
      uid_t i_uid;
      gid_t i_gid;
      loff_t i_size;
      struct timespec i_atime;
      struct timespec i_mtime;
      struct timespec i_ctime;
      struct inode_operations *i_op;
      struct file_operations *i_fop;
      struct super_block *i_sb;
      void *i_private;
  };

  // 路径查找缓存（dcache）
  struct dentry *path_lookup(const char *pathname)
  {
      struct dentry *dentry = dcache_lookup(pathname);
      if (dentry) return dentry;

      // 缓存未命中，实际查找
      dentry = do_path_lookup(pathname);
      if (dentry) dcache_add(pathname, dentry);

      return dentry;
  }

  任务5.2：实现ext2文件系统（4周）
  任务5.3：实现块设备层（2周）
  任务5.4：实现缓冲区缓存（buffer cache）（1周）
  任务5.5：实现页缓存（page cache）（1周）

  阶段5验收标准：
  - ⬜ VFS层功能完善
  - ⬜ ext2文件系统能正常读写
  - ⬜ 块设备层工作正常
  - ⬜ 缓存机制提高I/O性能

  ---
  阶段6：高级特性与测试（2-4个月）

  任务6.1：实现信号机制（3周）
  任务6.2：实现管道（pipe）（1周）
  任务6.3：实现设备驱动框架（2周）
  任务6.4：网络栈基础（选做，4周+）
  任务6.5：测试套件（持续）

  阶段6验收标准：
  - ⬜ 信号机制完整工作
  - ⬜ 管道通信正常
  - ⬜ 设备驱动框架可用
  - ⬜ 通过基本POSIX测试

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
  | 阶段3: 系统调用 | ⬜ 待开始 | - | - | syscall framework |
  | 阶段4: C库移植 | ⬜ 待开始 | - | - | newlib/musl |
  | 阶段5: 文件系统 | ⬜ 待开始 | - | - | ext2/VFS完善 |
  | 阶段6: 高级特性 | ⬜ 待开始 | - | - | 信号/管道/驱动 |
