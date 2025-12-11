● MinixRV64到POSIX兼容系统的完整技术路线

  📋 总览

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
  阶段4：C标准库移植（1-2个月）

  任务4.1：选择C库（1周调研）

  选项对比：

  | C库     | 优点                 | 缺点              | 推荐度   |
  |--------|--------------------|-----------------|-------|
  | newlib | 专为嵌入式设计，体积小，易于移植   | 功能较少，不完全兼容POSIX | ⭐⭐⭐⭐⭐ |
  | musl   | 完整POSIX支持，代码简洁，性能好 | 需要更多系统调用支持      | ⭐⭐⭐⭐  |
  | glibc  | 功能最完整，兼容性最好        | 体积大，复杂度高，难以移植   | ⭐⭐    |

  推荐：newlib（阶段性目标）→ musl（长期目标）

  任务4.2：移植newlib（3-4周）

  步骤：

  1. 准备工作
  # 下载newlib
  wget ftp://sourceware.org/pub/newlib/newlib-4.3.0.tar.gz
  tar xf newlib-4.3.0.tar.gz
  cd newlib-4.3.0
  2. 实现系统调用stubs
  /* libgloss/riscv/syscalls.c */

  // newlib需要的系统调用接口
  int _open(const char *name, int flags, int mode)
  {
      return syscall(SYS_open, name, flags, mode);
  }

  int _close(int fd)
  {
      return syscall(SYS_close, fd);
  }

  int _read(int fd, void *buf, size_t count)
  {
      return syscall(SYS_read, fd, buf, count);
  }

  int _write(int fd, const void *buf, size_t count)
  {
      return syscall(SYS_write, fd, buf, count);
  }

  int _fstat(int fd, struct stat *st)
  {
      return syscall(SYS_fstat, fd, st);
  }

  int _lseek(int fd, off_t offset, int whence)
  {
      return syscall(SYS_lseek, fd, offset, whence);
  }

  int _isatty(int fd)
  {
      return 1;  // 简化实现
  }

  void *_sbrk(ptrdiff_t incr)
  {
      extern char end;  // 由链接器提供
      static char *heap_end = &end;
      char *prev_heap_end = heap_end;

      // 使用brk系统调用
      if (syscall(SYS_brk, heap_end + incr) < 0)
          return (void *)-1;

      heap_end += incr;
      return prev_heap_end;
  }

  int _kill(int pid, int sig)
  {
      return syscall(SYS_kill, pid, sig);
  }

  int _getpid(void)
  {
      return syscall(SYS_getpid);
  }
  3. 配置和编译
  mkdir build-newlib
  cd build-newlib

  ../configure \
      --target=riscv64-unknown-elf \
      --prefix=/opt/riscv64-minix \
      --disable-multilib \
      --enable-newlib-io-long-long \
      --enable-newlib-register-fini \
      --disable-newlib-supplied-syscalls

  make -j$(nproc)
  make install
  4. 创建启动代码
  /* crt0.S - C runtime startup */

  .section .text.init
  .global _start
  _start:
      // 清空bss段
      la t0, __bss_start
      la t1, __bss_end
  1:  sd zero, 0(t0)
      addi t0, t0, 8
      blt t0, t1, 1b

      // 设置栈指针（由内核传入）
      // sp已经由内核设置好

      // 设置全局指针
      .option push
      .option norelax
      la gp, __global_pointer$
      .option pop

      // 调用全局构造函数
      call __libc_init_array

      // 调用main（argc, argv, envp由内核传入）
      // a0 = argc, a1 = argv, a2 = envp
      call main

      // 调用exit（main返回值在a0）
      call exit

      // 不应该到达这里
  1:  j 1b

  任务4.3：测试C库功能（1周）

  创建测试程序：

  /* userspace/test_libc.c */

  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <fcntl.h>

  int main(int argc, char **argv)
  {
      printf("Hello from userspace!\n");
      printf("argc = %d\n", argc);

      // 测试文件操作
      int fd = open("/test.txt", O_WRONLY | O_CREAT, 0644);
      if (fd < 0) {
          perror("open");
          return 1;
      }

      const char *msg = "Hello, file system!\n";
      write(fd, msg, strlen(msg));
      close(fd);

      // 测试malloc
      char *buf = malloc(1024);
      strcpy(buf, "Dynamic memory works!");
      printf("%s\n", buf);
      free(buf);

      // 测试fork
      pid_t pid = fork();
      if (pid == 0) {
          printf("Child process: pid=%d\n", getpid());
          exit(0);
      } else {
          printf("Parent process: child pid=%d\n", pid);
          int status;
          wait(&status);
          printf("Child exited with status %d\n", status);
      }

      return 0;
  }

  编译：
  riscv64-unknown-elf-gcc -o test_libc test_libc.c \
      -nostartfiles -nostdlib \
      -L/opt/riscv64-minix/lib \
      -lc -lgcc \
      /opt/riscv64-minix/lib/crt0.o

  阶段4验收标准：
  - ⬜ newlib成功编译和链接
  - ⬜ printf/scanf等stdio函数工作
  - ⬜ malloc/free正常工作
  - ⬜ 能编译和运行简单的用户程序

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
