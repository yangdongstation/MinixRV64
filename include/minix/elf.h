/* MinixRV64 Donz Build - ELF Definitions
 *
 * ELF64 format for RISC-V
 * Following HowToFitPosix.md Stage 2 design
 */

#ifndef _MINIX_ELF_H
#define _MINIX_ELF_H

#include <types.h>

/* ============================================
 * ELF Types
 * ============================================ */
typedef unsigned long   Elf64_Addr;
typedef unsigned long   Elf64_Off;
typedef unsigned short  Elf64_Half;
typedef unsigned int    Elf64_Word;
typedef int             Elf64_Sword;
typedef unsigned long   Elf64_Xword;
typedef long            Elf64_Sxword;

/* ============================================
 * ELF Magic Numbers
 * ============================================ */
#define EI_NIDENT       16
#define ELFMAG0         0x7f
#define ELFMAG1         'E'
#define ELFMAG2         'L'
#define ELFMAG3         'F'

/* ELF class */
#define ELFCLASSNONE    0
#define ELFCLASS32      1
#define ELFCLASS64      2

/* ELF data encoding */
#define ELFDATANONE     0
#define ELFDATA2LSB     1   /* Little-endian */
#define ELFDATA2MSB     2   /* Big-endian */

/* ELF version */
#define EV_NONE         0
#define EV_CURRENT      1

/* ELF OS/ABI */
#define ELFOSABI_NONE   0   /* UNIX System V ABI */
#define ELFOSABI_LINUX  3   /* Linux */

/* ============================================
 * ELF Header (Elf64_Ehdr)
 * ============================================ */
typedef struct {
    unsigned char   e_ident[EI_NIDENT]; /* Magic number and info */
    Elf64_Half      e_type;             /* Object file type */
    Elf64_Half      e_machine;          /* Architecture */
    Elf64_Word      e_version;          /* Object file version */
    Elf64_Addr      e_entry;            /* Entry point virtual address */
    Elf64_Off       e_phoff;            /* Program header table offset */
    Elf64_Off       e_shoff;            /* Section header table offset */
    Elf64_Word      e_flags;            /* Processor-specific flags */
    Elf64_Half      e_ehsize;           /* ELF header size */
    Elf64_Half      e_phentsize;        /* Program header entry size */
    Elf64_Half      e_phnum;            /* Program header count */
    Elf64_Half      e_shentsize;        /* Section header entry size */
    Elf64_Half      e_shnum;            /* Section header count */
    Elf64_Half      e_shstrndx;         /* Section name string table index */
} Elf64_Ehdr;

/* e_type values */
#define ET_NONE         0   /* No file type */
#define ET_REL          1   /* Relocatable file */
#define ET_EXEC         2   /* Executable file */
#define ET_DYN          3   /* Shared object file */
#define ET_CORE         4   /* Core file */

/* e_machine values */
#define EM_NONE         0   /* No machine */
#define EM_RISCV        243 /* RISC-V */

/* ============================================
 * Program Header (Elf64_Phdr)
 * ============================================ */
typedef struct {
    Elf64_Word      p_type;     /* Segment type */
    Elf64_Word      p_flags;    /* Segment flags */
    Elf64_Off       p_offset;   /* Segment file offset */
    Elf64_Addr      p_vaddr;    /* Segment virtual address */
    Elf64_Addr      p_paddr;    /* Segment physical address */
    Elf64_Xword     p_filesz;   /* Segment size in file */
    Elf64_Xword     p_memsz;    /* Segment size in memory */
    Elf64_Xword     p_align;    /* Segment alignment */
} Elf64_Phdr;

/* p_type values */
#define PT_NULL         0   /* Program header table entry unused */
#define PT_LOAD         1   /* Loadable program segment */
#define PT_DYNAMIC      2   /* Dynamic linking information */
#define PT_INTERP       3   /* Program interpreter */
#define PT_NOTE         4   /* Auxiliary information */
#define PT_SHLIB        5   /* Reserved */
#define PT_PHDR         6   /* Entry for header table itself */
#define PT_TLS          7   /* Thread-local storage segment */

/* p_flags values */
#define PF_X            0x1 /* Execute */
#define PF_W            0x2 /* Write */
#define PF_R            0x4 /* Read */

/* ============================================
 * Section Header (Elf64_Shdr)
 * ============================================ */
typedef struct {
    Elf64_Word      sh_name;        /* Section name (string table index) */
    Elf64_Word      sh_type;        /* Section type */
    Elf64_Xword     sh_flags;       /* Section flags */
    Elf64_Addr      sh_addr;        /* Section virtual address */
    Elf64_Off       sh_offset;      /* Section file offset */
    Elf64_Xword     sh_size;        /* Section size in bytes */
    Elf64_Word      sh_link;        /* Link to another section */
    Elf64_Word      sh_info;        /* Additional section information */
    Elf64_Xword     sh_addralign;   /* Section alignment */
    Elf64_Xword     sh_entsize;     /* Entry size if section holds table */
} Elf64_Shdr;

/* sh_type values */
#define SHT_NULL        0   /* Section header table entry unused */
#define SHT_PROGBITS    1   /* Program data */
#define SHT_SYMTAB      2   /* Symbol table */
#define SHT_STRTAB      3   /* String table */
#define SHT_RELA        4   /* Relocation entries with addends */
#define SHT_HASH        5   /* Symbol hash table */
#define SHT_DYNAMIC     6   /* Dynamic linking information */
#define SHT_NOTE        7   /* Notes */
#define SHT_NOBITS      8   /* Program space with no data (bss) */
#define SHT_REL         9   /* Relocation entries */

/* sh_flags values */
#define SHF_WRITE       0x1     /* Writable */
#define SHF_ALLOC       0x2     /* Occupies memory during execution */
#define SHF_EXECINSTR   0x4     /* Executable */

/* ============================================
 * Auxiliary Vector (for exec)
 * ============================================ */
typedef struct {
    Elf64_Xword a_type;     /* Entry type */
    union {
        Elf64_Xword a_val;  /* Integer value */
    } a_un;
} Elf64_auxv_t;

/* Auxiliary vector types */
#define AT_NULL         0   /* End of vector */
#define AT_IGNORE       1   /* Entry should be ignored */
#define AT_EXECFD       2   /* File descriptor of program */
#define AT_PHDR         3   /* Program headers for program */
#define AT_PHENT        4   /* Size of program header entry */
#define AT_PHNUM        5   /* Number of program headers */
#define AT_PAGESZ       6   /* System page size */
#define AT_BASE         7   /* Base address of interpreter */
#define AT_FLAGS        8   /* Flags */
#define AT_ENTRY        9   /* Entry point of program */
#define AT_UID          11  /* Real uid */
#define AT_EUID         12  /* Effective uid */
#define AT_GID          13  /* Real gid */
#define AT_EGID         14  /* Effective gid */
#define AT_PLATFORM     15  /* String identifying CPU */
#define AT_RANDOM       25  /* Address of 16 random bytes */

/* ============================================
 * ELF Verification Macros
 * ============================================ */
#define ELF_MAGIC_OK(ehdr) \
    ((ehdr)->e_ident[0] == ELFMAG0 && \
     (ehdr)->e_ident[1] == ELFMAG1 && \
     (ehdr)->e_ident[2] == ELFMAG2 && \
     (ehdr)->e_ident[3] == ELFMAG3)

#define ELF_CLASS_OK(ehdr) \
    ((ehdr)->e_ident[4] == ELFCLASS64)

#define ELF_DATA_OK(ehdr) \
    ((ehdr)->e_ident[5] == ELFDATA2LSB)

#define ELF_MACHINE_OK(ehdr) \
    ((ehdr)->e_machine == EM_RISCV)

#define ELF_TYPE_OK(ehdr) \
    ((ehdr)->e_type == ET_EXEC || (ehdr)->e_type == ET_DYN)

/* Full ELF validation */
#define ELF_VALID(ehdr) \
    (ELF_MAGIC_OK(ehdr) && \
     ELF_CLASS_OK(ehdr) && \
     ELF_DATA_OK(ehdr) && \
     ELF_MACHINE_OK(ehdr) && \
     ELF_TYPE_OK(ehdr))

/* ============================================
 * ELF Loading Functions
 * ============================================ */

/* Load ELF executable */
int load_elf_binary(const char *path, struct task_struct *task);

/* Check if file is valid ELF */
int is_elf_binary(const void *data, unsigned long size);

#endif /* _MINIX_ELF_H */
