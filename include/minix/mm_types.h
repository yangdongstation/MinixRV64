/* MinixRV64 Donz Build - Memory Management Types
 *
 * Process memory space structures (mm_struct, vm_area_struct)
 * Following HowToFitPosix.md Stage 2 design
 */

#ifndef _MINIX_MM_TYPES_H
#define _MINIX_MM_TYPES_H

#include <types.h>
#include <minix/sched.h>

/* Forward declarations */
struct file;
struct task_struct;

/* Atomic type (simplified) */
typedef struct {
    volatile long counter;
} atomic_t;

#define ATOMIC_INIT(i)  { (i) }

static inline long atomic_read(const atomic_t *v)
{
    return v->counter;
}

static inline void atomic_set(atomic_t *v, long i)
{
    v->counter = i;
}

static inline void atomic_inc(atomic_t *v)
{
    v->counter++;
}

static inline void atomic_dec(atomic_t *v)
{
    v->counter--;
}

static inline int atomic_dec_and_test(atomic_t *v)
{
    return --v->counter == 0;
}

/* Spinlock (simplified for single CPU) */
typedef struct {
    volatile unsigned long lock;
} spinlock_t;

#define SPIN_LOCK_INIT { 0 }

static inline void spin_lock_init(spinlock_t *lock)
{
    lock->lock = 0;
}

static inline void spin_lock(spinlock_t *lock)
{
    while (__sync_lock_test_and_set(&lock->lock, 1)) {
        /* Spin */
    }
}

static inline void spin_unlock(spinlock_t *lock)
{
    __sync_lock_release(&lock->lock);
}

/* ============================================
 * VMA Flags
 * ============================================ */
#define VM_READ         0x00000001  /* Readable */
#define VM_WRITE        0x00000002  /* Writable */
#define VM_EXEC         0x00000004  /* Executable */
#define VM_SHARED       0x00000008  /* Shared mapping */
#define VM_MAYREAD      0x00000010  /* May be readable */
#define VM_MAYWRITE     0x00000020  /* May be writable */
#define VM_MAYEXEC      0x00000040  /* May be executable */
#define VM_GROWSDOWN    0x00000100  /* Stack grows down */
#define VM_DENYWRITE    0x00000800  /* Deny write access */
#define VM_LOCKED       0x00002000  /* Locked in memory */
#define VM_STACK        0x00000100  /* Stack area (same as GROWSDOWN) */

/* Page protection bits */
typedef unsigned long pgprot_t;

/* ============================================
 * Virtual Memory Area (VMA)
 * ============================================ */
struct vm_area_struct {
    unsigned long vm_start;         /* Start address (inclusive) */
    unsigned long vm_end;           /* End address (exclusive) */

    struct vm_area_struct *vm_next; /* Next VMA in list */
    struct vm_area_struct *vm_prev; /* Previous VMA in list */

    struct mm_struct *vm_mm;        /* Owning mm_struct */

    unsigned long vm_flags;         /* Flags and attributes */
    pgprot_t vm_page_prot;          /* Page protection bits */

    /* File mapping */
    struct file *vm_file;           /* Mapped file (NULL for anonymous) */
    unsigned long vm_pgoff;         /* File offset (in pages) */

    /* Private data */
    void *vm_private_data;
};

/* ============================================
 * Memory Descriptor (mm_struct)
 * ============================================ */

/* Auxiliary vector size */
#define AT_VECTOR_SIZE  46

struct mm_struct {
    unsigned long *pgd;             /* Page global directory */

    /* Code segment */
    unsigned long start_code;       /* Code start */
    unsigned long end_code;         /* Code end */

    /* Data segment */
    unsigned long start_data;       /* Data start */
    unsigned long end_data;         /* Data end */

    /* Heap */
    unsigned long start_brk;        /* Heap start */
    unsigned long brk;              /* Current heap end (brk position) */

    /* Stack */
    unsigned long start_stack;      /* Stack start (high address) */
    unsigned long arg_start;        /* Argument area start */
    unsigned long arg_end;          /* Argument area end */
    unsigned long env_start;        /* Environment start */
    unsigned long env_end;          /* Environment end */

    /* VMA management */
    struct vm_area_struct *mmap;    /* VMA list head */
    int map_count;                  /* Number of VMAs */
    unsigned long total_vm;         /* Total VM size (pages) */
    unsigned long locked_vm;        /* Locked memory */
    unsigned long data_vm;          /* Data segment size */
    unsigned long exec_vm;          /* Executable segment size */
    unsigned long stack_vm;         /* Stack size */

    /* Reference counting */
    atomic_t mm_users;              /* Users of this mm */
    atomic_t mm_count;              /* Reference count */

    /* Locks */
    spinlock_t page_table_lock;     /* Page table lock */

    /* For exec */
    unsigned long saved_auxv[AT_VECTOR_SIZE];
};

/* ============================================
 * Helper Functions
 * ============================================ */

/* Allocate a new mm_struct */
struct mm_struct *mm_alloc(void);

/* Free mm_struct */
void mm_free(struct mm_struct *mm);

/* Duplicate mm_struct (for fork) */
struct mm_struct *dup_mm(struct task_struct *tsk);

/* Release mm reference */
void mmput(struct mm_struct *mm);

/* Get mm reference */
void mmget(struct mm_struct *mm);

/* Exit mm (called on process exit) */
void exit_mm(struct task_struct *tsk);

/* Allocate VMA */
struct vm_area_struct *vm_area_alloc(struct mm_struct *mm);

/* Free VMA */
void vm_area_free(struct vm_area_struct *vma);

/* Insert VMA into mm */
int insert_vm_area(struct mm_struct *mm, struct vm_area_struct *vma);

/* Find VMA containing address */
struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr);

/* Copy page range for COW */
int copy_page_range(struct mm_struct *dst, struct mm_struct *src,
                    struct vm_area_struct *vma);

/* Handle COW fault */
int do_cow_fault(struct vm_area_struct *vma, unsigned long address);

/* Check if mapping is COW */
static inline int is_cow_mapping(unsigned long flags)
{
    return (flags & (VM_SHARED | VM_MAYWRITE)) == VM_MAYWRITE;
}

#endif /* _MINIX_MM_TYPES_H */
