# MinixRV64 Donz Build 第二部分核心刹车检查点

基于musl+Linux ABI方案的"踩刹车"清单，每个检查点都是**AI最容易出错且会导致灾难性后果**的陷阱。

---

## **任务2.1：进程数据结构设计（1周）**

### **刹车点1：thread_info栈底对齐**
```c
// 在 include/minix/thread_info.h
struct thread_info {
    struct task_struct *task;  // Must be first member
    ...
};
```
**踩刹车原因**：AI常忘记`thread_info`必须位于内核栈**物理底部**，且`sp & THREAD_MASK`计算必须精确。错1字节就导致`current`宏取到垃圾指针， panic时连栈回溯都失效。

**检查清单**：
- [ ] `THREAD_SIZE`必须是`PAGE_SIZE`的2的幂倍数（2 * PAGE_SIZE = 8192）
- [ ] `alloc_thread_info()`返回的地址是否**2页对齐**？
- [ ] `current_thread_info()`的汇编实现是否用`sp & ~(THREAD_SIZE-1)`而非`%`运算？

**致命性**：⭐⭐⭐⭐⭐（无法调试的随机崩溃）

---

## **任务2.2：fork的COW实现（2周）**

### **刹车点2：父进程页表修改与TLB刷新的原子性**
```c
// 在 copy_page_range()
set_pte(src_pte, pte_wrprotect(pte));  // 父进程变只读
flush_tlb_mm(src_mm);                  // TLB刷新
```
**踩刹车原因**：AI会生成正确顺序，但**遗漏关中断**。若在`set_pte`和`flush_tlb`之间发生时钟中断，调度到另一个CPU核可能读到旧TLB项，引发**silent data corruption**。

**检查清单**：
- [ ] `set_pte`和`flush_tlb_mm`是否被`local_irq_save()/restore()`包裹？
- [ ] `get_page(old_page)`是否在`set_pte`之前完成？（防止page在引用前被释放）
- [ ] **错误路径**：如果`pte_alloc()`失败，是否回滚已复制的PTE并`put_page`？

**致命性**：⭐⭐⭐⭐⭐（数据静默损坏，无法重现）

### **刹车点3：子进程返回0的寄存器设置**
```c
// 在 copy_thread()
childregs->a0 = 0;  // fork()在子进程返回0
```
**踩刹车原因**：AI常把`a0 = 0`写在`copy_process()`的C代码里，但**必须在汇编层面**修改`pt_regs`。否则子进程从内核返回用户态时，`a0`会被`schedule()`覆盖。

**检查清单**：
- [ ] `copy_thread()`是否操作`struct pt_regs *childregs = task_pt_regs(p)`？
- [ ] `childregs->a0 = 0`是否在`switch_to()`的`ld ra/s0-s11`之前设置？
- [ ] `pt_regs`是否位于**子进程内核栈顶部**（`kernel_sp - sizeof(pt_regs)`）？

**致命性**：⭐⭐⭐⭐（fork返回值错误，用户态逻辑混乱）

---

## **任务2.3：调度器O(1)实现（2周）**

### **刹车点4：优先级数组位图的原子操作**
```c
// 在 prio_array 的 bitmap 操作
__set_bit(prio, array->bitmap);
```
**踩刹车原因**：AI会用普通`array->bitmap[word] |= mask`，但调度器可能被**时钟中断抢占**。两个核同时设置不同bit会导致一个核的更新丢失，引发优先级反转（高prio任务饿死）。

**检查清单**：
- [ ] 是否使用`__set_bit()`内建函数（带`LOCK`前缀原子指令）？
- [ ] `enqueue_task()`是否用`spin_lock_irqsave(&rq->lock)`保护整个操作？
- [ ] `find_first_bit()`是否关中断调用？（防止位图在中断处理中被修改）

**致命性**：⭐⭐⭐⭐（随机调度延迟，难定位）

---

## **任务2.4：ELF加载器（2周）**

### **刹车点5：auxv辅助向量的栈布局**
```c
// 在 create_elf_tables()
NEW_AUX_ENT(AT_PHDR, load_addr + elf_ex->e_phoff);
```
**踩刹车原因**：AI会正确填充auxv，但**栈指针p的计算错误**。musl依赖auxv的精确位置找到程序入口和页大小，若p未16字节对齐或auxv未以AT_NULL结尾，**musl启动崩溃**，栈回溯显示在`__libc_start_main`内。

**检查清单**：
- [ ] `bprm->p`是否每次`NEW_AUX_ENT`后减去16字节（2个unsigned long）？
- [ ] `bprm->p`最终是否`& ~15`（16字节对齐）？
- [ ] 是否最后添加`{AT_NULL, 0}`？
- [ ] 启动后`sp`寄存器是否指向`argc`（而非auxv）？

**致命性**：⭐⭐⭐⭐⭐（用户态完全无法启动，无内核日志）

### **刹车点6：PT_LOAD段权限与MMU标志映射**
```c
// 在 elf_map()
if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
```
**踩刹车原因**：AI会机械转换`PF_W → PROT_WRITE`，但**遗漏PROT_READ**。ELF要求可写段必须可读，否则`do_page_fault`会因权限不足错误触发SIGSEGV，而用户程序无法调试（gdb无法附加）。

**检查清单**：
- [ ] 是否强制`if (PF_W) prot |= (PROT_READ|PROT_WRITE)`？
- [ ] 代码段（PF_X）是否设置PROT_EXEC且**不设置PROT_WRITE**？（W^X安全）
- [ ] `vm_flags`是否同时设置`VM_READ|VM_MAYREAD`？（vma权限检查）

**致命性**：⭐⭐⭐⭐（段错误无法调试，用户态开发停滞）

---

## **任务3.1：系统调用框架（2周）**

### **刹车点7：ecall入口的寄存器保存顺序**
```c
// 在 handle_syscall 的 SAVE_ALL
sd a0, PT_A0(sp)
sd a1, PT_A1(sp)
...
```
**踩刹车原因**：AI生成的汇编保存顺序可能与`struct pt_regs`定义不一致。若`PT_A7`偏移错误，系统调用号读取错误，导致`sys_openat`被当成`sys_close`执行，**用户态文件操作随机失败**。

**检查清单**：
- [ ] `arch/riscv64/include/asm/ptrace.h`中`PT_A7`偏移是否为`8*17`（a0-a7是x10-x17）？
- [ ] `save_all_regs`和`restore_all_regs`是否**严格对称**？
- [ ] `handle_syscall`是否先`save_all_regs`再读`PT_A7`？

**致命性**：⭐⭐⭐⭐⭐（系统调用错乱，症状诡异难排查）

---

### **刹车点8：未实现系统调用的ENOSYS返回**
```c
syscall_enosys:
    li a0, -ENOSYS
    j syscall_return
```
**踩刹车原因**：AI可能遗漏`syscall_enosys`标签，或返回正数。musl遇到非负返回值会**重复调用**，导致无限循环，CPU 100%占用。

**检查清单**：
- [ ] 返回时`a0`是否为**负数**（`-ENOSYS = -38`）？
- [ ] `syscall_return`是否走`restore_all_regs`路径（而非直接`sret`）？
- [ ] `__NR_syscalls`定义是否**恰好等于**表最大索引+1？

**致命性**：⭐⭐⭐⭐（系统调用死循环，系统卡死）

---

## **任务4.2：musl交叉编译**

### **刹车点9：工具链的Linux ABI一致性**
```bash
# 在 musl-cross-make 配置
TARGET = riscv64-linux-musl
```
**踩刹车原因**：AI可能写成`riscv64-unknown-elf`，生成的**glibc abi**与kernel的**Linux syscall abi**不兼容。链接时报错`undefined reference to '__errno_location'`，musl无法启动。

**检查清单**：
- [ ] `riscv64-linux-musl-gcc -dumpmachine`输出是否为`riscv64-linux-musl`？
- [ ] `musl-gcc`的`--sysroot`是否指向Linux头文件（非bare-metal）？
- [ ] `ldd ./hello`是否显示`statically linked`？（验证-static生效）

**致命性**：⭐⭐⭐⭐⭐（用户态完全无法编译，项目停滞）

---

## **任务5.1：VFS路径解析**

### **刹车点10：dentry缓存哈希冲突**
```c
// 在 dcache_lookup()
unsigned int hash = full_name_hash(parent, name->name, name->len);
```
**踩刹车原因**：AI生成的哈希函数可能不处理`name_len=0`的情况，导致`/`根目录的dentry被错误缓存，后续所有绝对路径解析返回`-ENOENT`。

**检查清单**：
- [ ] `full_name_hash`是否处理`len=0`返回`0`？
- [ ] `d_lookup()`是否比较`parent`指针（不仅是hash）？
- [ ] `d_invalidate()`是否在unlink/rmdir时被调用？（防止 stale dentry）

**致命性**：⭐⭐⭐⭐（文件系统"幽灵文件"现象）

---

## **任务5.2：ext2块分配**

### **刹车点11：块位图与块组描述符的同步**
```c
// 在 ext2_alloc_block()
mark_buffer_dirty(bh);  // 标记位图脏
```
**踩刹车原因**：AI会标记位图脏，但**忘记标记组描述符脏**。崩溃后重启，`s_free_blocks_count`未回写，导致ext2认为有块可用但实际已分配，写入时破坏数据。

**检查清单**：
- [ ] 分配块后是否`mark_buffer_dirty(gdp_bh)`更新`bg_free_blocks_count`？
- [ ] 是否同时`mark_inode_dirty(inode)`更新`i_blocks`？
- [ ] `sync_blockdev()`是否 flush 所有脏buffer和inode？

**致命性**：⭐⭐⭐⭐⭐（文件系统崩溃后无法恢复）

---

## **任务6.1：信号sigreturn**

### **刹车点12：信号栈帧的 RA 覆盖陷阱**
```c
// 在 setup_rt_frame()
frame->uc.uc_mcontext.sc_regs[1] = (unsigned long)&frame->retcode;
```
**踩刹车原因**：AI可能将ra(s0)恢复值指向`&frame->retcode`，但信号处理函数是C函数，会使用栈。**若信号处理函数调用深度>1，`retcode`会被覆盖**，sigreturn时跳转到垃圾地址，用户态segmentation fault无法捕获。

**检查清单**：
- [ ] `retcode`是否放在`rt_sigframe`结构体的**最底部**（高地址）？
- [ ] `retcode`是否8字节对齐且用`volatile`标记？
- [ ] `sc_regs[1]`是否指向`frame->retcode`而非栈上动态值？

**致命性**：⭐⭐⭐⭐⭐（信号处理返回后100%崩溃）

---

## **任务6.2：管道pipe**

### **刹车点13：读写端的fasync异步通知**
```c
// 在 pipe_write()
wake_up_interruptible(&pipe->wait);
```
**踩刹车原因**：AI会唤醒等待进程，但**遗漏`kill_fasync()`**。当读端用`select()`监听时，无法收到POLL_IN事件，导致管道通信死锁（双方都阻塞）。

**检查清单**：
- [ ] `pipe->wait`是否关联`fasync_struct`？
- [ ] `pipe_read()`是否在数据到达后`kill_fasync(&pipe->fasync, SIGIO, POLL_IN)`？
- [ ] `pipe_write()`是否在空间可用后`kill_fasync(&pipe->fasync, SIGIO, POLL_OUT)`？

**致命性**：⭐⭐⭐（影响select/poll，但不影响基本read/write）

---

## **总结：踩刹车优先级矩阵**

| 任务 | 刹车点 | 致命性 | 隐蔽性 | 检查优先级 |
|------|--------|--------|--------|------------|
| fork COW | TLB原子性 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | **P0** |
| ELF auxv | 栈布局 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | **P0** |
| syscall入口 | 寄存器保存 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | **P0** |
| ext2块分配 | 位图同步 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | **P0** |
| 信号sigreturn | retcode覆盖 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | **P0** |
| musl工具链 | ABI一致性 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | **P0** |
| 调度器 | 位图原子性 | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | **P1** |
| VFS dentry | 哈希冲突 | ⭐⭐⭐⭐ | ⭐⭐⭐ | **P1** |
| 管道fasync | 异步通知 | ⭐⭐⭐ | ⭐⭐⭐ | **P2** |

**操作建议**：每天开工前，操作员先读一遍当天任务的**P0级刹车点**，生成代码后逐条check。架构师只抽查P0，其他信任操作员。
