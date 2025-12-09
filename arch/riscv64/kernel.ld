/* Linker script for Minix RV64 on MilkV Duo CV1800B */

OUTPUT_ARCH(riscv)
ENTRY(_start)

/* Memory layout for CV1800B */
MEMORY {
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 64M   /* First 64MB for kernel */
    IO   (rw) : ORIGIN = 0x07000000, LENGTH = 16M   /* I/O registers */
}

/* Sections */
SECTIONS {
    . = 0x80000000;  /* Kernel load address */

    /* Boot code and data */
    .text : {
        *(.text.init)
        *(.text)
    } > RAM

    /* Read-only data */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    } > RAM

    /* Initialized data */
    .data : {
        *(.data)
        *(.data.*)
    } > RAM

    /* Uninitialized data */
    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(.sbss.*)
        __bss_end = .;
    } > RAM

    /* Stack */
    . = ALIGN(16);
    .stack : {
        __stack_start = .;
        . = . + 64K;  /* 64KB stack */
        __stack_end = .;
    } > RAM

    /* Kernel heap (initial) */
    . = ALIGN(16);
    __heap_start = .;

    /* Discard sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}