/* Linker script for Minix RV64 on QEMU virt machine */

OUTPUT_ARCH(riscv)
ENTRY(_start)

/* Sections */
SECTIONS {
    /* Place kernel at start of RAM (no OpenSBI, bare metal) */
    . = 0x80000000;

    /* Boot code and data */
    .text : {
        *(.text.init)
        *(.text)
        *(.text.*)
    }

    /* Read-only data */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    /* Initialized data */
    .data : {
        *(.data)
        *(.data.*)
        *(.sdata)
        *(.sdata.*)
    }

    /* Uninitialized data */
    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(.sbss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* Stack */
    . = ALIGN(16);
    .stack : {
        __stack_start = .;
        . = . + 64K;  /* 64KB stack */
        __stack_end = .;
    }

    /* Kernel heap (initial) */
    . = ALIGN(16);
    __heap_start = .;

    /* Symbol for end of kernel */
    _end = .;

    /* Discard sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.comment)
        *(.gnu*)
    }
}