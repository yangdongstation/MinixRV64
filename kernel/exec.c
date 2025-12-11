/* MinixRV64 Donz Build - ELF Loader
 *
 * Load and execute ELF binaries
 * Following HowToFitPosix.md Stage 2 design
 */

#include <minix/config.h>
#include <minix/task.h>
#include <minix/mm.h>
#include <minix/elf.h>
#include <types.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Error codes */
#define ENOEXEC     8   /* Exec format error */
#define ENOMEM      12  /* Out of memory */
#define ENOENT      2   /* No such file */

/* External functions */
extern void early_puts(const char *s);
extern void early_puthex(unsigned long val);
extern unsigned long alloc_pages(int order);
extern void free_pages(unsigned long addr, int order);
extern void *kmalloc(unsigned long size);
extern void kfree(void *ptr);

/* Page size */
#define PAGE_SIZE       4096
#define PAGE_SHIFT      12
#define PAGE_MASK       (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x)   (((x) + PAGE_SIZE - 1) & PAGE_MASK)

/* User address space layout */
#define USER_STACK_TOP      0x7FFFFFFFF000UL    /* Top of user stack */
#define USER_STACK_SIZE     0x100000            /* 1MB stack */
#define USER_HEAP_START     0x10000000UL        /* Start of heap */

/* ============================================
 * Memory Copy Helper
 * ============================================ */

static void memcpy_local(void *dst, const void *src, unsigned long n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) {
        *d++ = *s++;
    }
}

static void memset_local(void *dst, int c, unsigned long n)
{
    unsigned char *d = (unsigned char *)dst;
    while (n--) {
        *d++ = (unsigned char)c;
    }
}

/* ============================================
 * ELF Validation
 * ============================================ */

int is_elf_binary(const void *data, unsigned long size)
{
    const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *)data;

    if (size < sizeof(Elf64_Ehdr)) {
        return 0;
    }

    return ELF_VALID(ehdr);
}

/* ============================================
 * Setup User Stack
 *
 * Stack layout at exec:
 *
 * High address
 * ├─────────────────────────┤
 * │    Environment strings  │
 * ├─────────────────────────┤
 * │    Argument strings     │
 * ├─────────────────────────┤
 * │         NULL            │ ← end of auxv
 * │    Auxiliary Vector     │
 * │         ...             │
 * ├─────────────────────────┤
 * │         NULL            │ ← end of envp
 * │    envp[envc-1]         │
 * │         ...             │
 * │    envp[0]              │
 * ├─────────────────────────┤
 * │         NULL            │ ← end of argv
 * │    argv[argc-1]         │
 * │         ...             │
 * │    argv[0]              │
 * ├─────────────────────────┤
 * │       argc              │ ← sp points here
 * └─────────────────────────┘
 * Low address
 *
 * ============================================ */

static unsigned long setup_user_stack(struct mm_struct *mm,
                                       char **argv, char **envp,
                                       Elf64_Ehdr *ehdr,
                                       unsigned long load_addr)
{
    unsigned long stack_top = USER_STACK_TOP;
    unsigned long stack_page;
    unsigned long sp;
    int argc = 0, envc = 0;

    /* Suppress unused warnings for future use */
    (void)ehdr;
    (void)load_addr;

    /* Count arguments */
    if (argv) {
        while (argv[argc]) argc++;
    }
    if (envp) {
        while (envp[envc]) envc++;
    }

    /* Allocate stack pages (4 pages = 16KB) */
    stack_page = alloc_pages(2);  /* 4 pages */
    if (!stack_page) {
        early_puts("[ELF] Failed to allocate user stack\n");
        return 0;
    }

    /* Setup mm stack info */
    mm->start_stack = stack_top - USER_STACK_SIZE;

    /* For now, we work in kernel space since we don't have full VM yet */
    /* In real implementation, this would be mapped to user space */

    sp = stack_page + (4 * PAGE_SIZE);  /* Top of allocated stack */

    /* Copy strings to stack (environment first, then arguments) */
    /* This is simplified - real implementation needs proper user mapping */

    /* For now, just setup minimal stack with argc = 0 */
    sp -= 8;  /* Align */
    sp &= ~0xF;  /* 16-byte alignment */

    /* Push NULL for envp terminator */
    sp -= 8;
    *(unsigned long *)sp = 0;

    /* Push NULL for argv terminator */
    sp -= 8;
    *(unsigned long *)sp = 0;

    /* Push argc */
    sp -= 8;
    *(unsigned long *)sp = argc;

    early_puts("[ELF] User stack setup at ");
    early_puthex(sp);
    early_puts("\n");

    return sp;
}

/* ============================================
 * Load ELF Segments
 * ============================================ */

static int load_elf_segments(const void *elf_data, unsigned long elf_size,
                             struct mm_struct *mm,
                             unsigned long *entry_point,
                             unsigned long *load_addr)
{
    const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *)elf_data;
    const Elf64_Phdr *phdr;
    int i;
    unsigned long min_addr = ~0UL;
    unsigned long max_addr = 0;

    *entry_point = ehdr->e_entry;
    *load_addr = 0;

    /* Process each program header */
    for (i = 0; i < ehdr->e_phnum; i++) {
        phdr = (const Elf64_Phdr *)((const char *)elf_data +
                                     ehdr->e_phoff +
                                     i * ehdr->e_phentsize);

        /* Only load PT_LOAD segments */
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        /* Validate segment */
        if (phdr->p_offset + phdr->p_filesz > elf_size) {
            early_puts("[ELF] Invalid segment offset\n");
            return -ENOEXEC;
        }

        /* Track address range */
        if (phdr->p_vaddr < min_addr) {
            min_addr = phdr->p_vaddr;
        }
        if (phdr->p_vaddr + phdr->p_memsz > max_addr) {
            max_addr = phdr->p_vaddr + phdr->p_memsz;
        }

        early_puts("[ELF] Loading segment: vaddr=");
        early_puthex(phdr->p_vaddr);
        early_puts(" filesz=");
        early_puthex(phdr->p_filesz);
        early_puts(" memsz=");
        early_puthex(phdr->p_memsz);
        early_puts(" flags=");
        early_puthex(phdr->p_flags);
        early_puts("\n");

        /* Allocate pages for segment */
        unsigned long seg_pages = (PAGE_ALIGN(phdr->p_vaddr + phdr->p_memsz) -
                                   (phdr->p_vaddr & PAGE_MASK)) >> PAGE_SHIFT;
        if (seg_pages == 0) seg_pages = 1;

        /* Calculate order for allocation */
        int order = 0;
        unsigned long p = 1;
        while (p < seg_pages) {
            p <<= 1;
            order++;
        }

        unsigned long segment_mem = alloc_pages(order);
        if (!segment_mem) {
            early_puts("[ELF] Failed to allocate segment memory\n");
            return -ENOMEM;
        }

        /* Zero the segment memory */
        memset_local((void *)segment_mem, 0, p << PAGE_SHIFT);

        /* Copy segment data */
        if (phdr->p_filesz > 0) {
            memcpy_local((void *)(segment_mem + (phdr->p_vaddr & (PAGE_SIZE - 1))),
                        (const char *)elf_data + phdr->p_offset,
                        phdr->p_filesz);
        }

        /* BSS is already zeroed */

        /* TODO: Map to user address space with proper permissions */
        /* For now we just track the mm info */
        if (phdr->p_flags & PF_X) {
            if (mm->start_code == 0) {
                mm->start_code = phdr->p_vaddr;
            }
            mm->end_code = phdr->p_vaddr + phdr->p_filesz;
        } else {
            if (mm->start_data == 0 && (phdr->p_flags & PF_W)) {
                mm->start_data = phdr->p_vaddr;
            }
            mm->end_data = phdr->p_vaddr + phdr->p_filesz;
        }
    }

    *load_addr = min_addr;
    mm->start_brk = PAGE_ALIGN(max_addr);
    mm->brk = mm->start_brk;

    early_puts("[ELF] Entry point: ");
    early_puthex(*entry_point);
    early_puts("\n");

    return 0;
}

/* ============================================
 * Load ELF Binary
 * ============================================ */

int load_elf_binary(const char *path, struct task_struct *task)
{
    /* TODO: Read file from VFS */
    /* For now, this is a stub that would be called by execve */

    (void)path;  /* Will be used when VFS is implemented */

    early_puts("[ELF] load_elf_binary called\n");

    if (!task) {
        return -1;
    }

    /* In full implementation:
     * 1. Open file via VFS
     * 2. Read ELF header
     * 3. Validate ELF
     * 4. Create new mm_struct
     * 5. Load segments
     * 6. Setup user stack
     * 7. Setup trapframe for return to user mode
     */

    return -ENOEXEC;  /* Not implemented yet */
}

/* ============================================
 * Load ELF from Memory Buffer
 *
 * Used when ELF is already in memory (e.g., embedded)
 * ============================================ */

int load_elf_from_memory(const void *elf_data, unsigned long elf_size,
                         struct task_struct *task,
                         char **argv, char **envp)
{
    const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *)elf_data;
    struct mm_struct *mm;
    unsigned long entry_point;
    unsigned long load_addr;
    unsigned long sp;
    int ret;

    /* Validate ELF */
    if (!is_elf_binary(elf_data, elf_size)) {
        early_puts("[ELF] Invalid ELF binary\n");
        return -ENOEXEC;
    }

    early_puts("[ELF] Loading ELF binary, size=");
    early_puthex(elf_size);
    early_puts("\n");

    /* Allocate new mm_struct */
    mm = mm_alloc();
    if (!mm) {
        early_puts("[ELF] Failed to allocate mm_struct\n");
        return -ENOMEM;
    }

    /* Load segments */
    ret = load_elf_segments(elf_data, elf_size, mm, &entry_point, &load_addr);
    if (ret < 0) {
        mm_free(mm);
        return ret;
    }

    /* Setup user stack */
    sp = setup_user_stack(mm, argv, envp, (Elf64_Ehdr *)ehdr, load_addr);
    if (!sp) {
        mm_free(mm);
        return -ENOMEM;
    }

    /* Replace task's mm */
    if (task->mm) {
        /* TODO: Properly free old mm */
    }
    task->mm = mm;
    task->active_mm = mm;

    /* Setup trapframe for return to user mode */
    if (task->trapframe) {
        task->trapframe->sepc = entry_point;
        task->trapframe->sp = sp;
        task->trapframe->a0 = 0;  /* argc will be on stack */

        /* Set sstatus for user mode */
        /* SPP=0 (return to user), SPIE=1 (enable interrupts on return) */
        task->trapframe->sstatus = 0x20;  /* SPIE=1 */
    }

    early_puts("[ELF] Binary loaded successfully\n");

    return 0;
}

/* ============================================
 * Execve System Call
 * ============================================ */

int do_execve(const char *filename, char **argv, char **envp)
{
    struct task_struct *curr = get_current();

    /* Suppress unused warnings - will be used when implemented */
    (void)filename;
    (void)argv;
    (void)envp;

    if (!curr) {
        return -1;
    }

    early_puts("[EXEC] do_execve called\n");

    /* TODO: Implementation
     * 1. Open file via VFS
     * 2. Read into buffer
     * 3. Call load_elf_from_memory or load_elf_binary
     * 4. On success, return to user mode won't return here
     */

    return -ENOEXEC;
}

/* Kernel execve - for starting init process */
int kernel_execve(const char *filename, char **argv, char **envp)
{
    early_puts("[EXEC] kernel_execve: ");
    early_puts(filename);
    early_puts("\n");

    return do_execve(filename, argv, envp);
}

/* sys_execve - execve system call wrapper */
long sys_execve(const char *filename, char **argv, char **envp)
{
    return do_execve(filename, argv, envp);
}
