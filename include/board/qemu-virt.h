/* QEMU virt board definitions for RISC-V 64-bit */

#ifndef _BOARD_QEMU_VIRT_H
#define _BOARD_QEMU_VIRT_H

/* Physical memory map for QEMU virt machine */
#define QEMU_VIRT_FLASH_BASE   0x20000000UL
#define QEMU_VIRT_FLASH_SIZE   (64 * 1024 * 1024)  /* 64MB */
#define QEMU_VIRT_DRAM_BASE    0x80000000UL
#define QEMU_VIRT_DRAM_SIZE    (128 * 1024 * 1024)  /* 128MB minimum */

/* VirtIO device base addresses */
#define VIRTIO_BASE           0x10001000UL
#define VIRTIO_SIZE           0x1000

/* PLIC (Platform-Level Interrupt Controller) */
#define PLIC_BASE             0x0c000000UL
#define PLIC_PRIORITY_OFFSET  0x0
#define PLIC_PENDING_OFFSET   0x1000
#define PLIC_ENABLE_OFFSET    0x2000
#define PLIC_THRESHOLD_OFFSET 0x200000
#define PLIC_CLAIM_OFFSET     0x200004

/* CLINT (Core Local Interruptor) */
#define CLINT_BASE            0x02000000UL
#define CLINT_MSIP_OFFSET     0x0000UL
#define CLINT_MTIMECMP_OFFSET 0x4000UL
#define CLINT_MTIME_OFFSET    0xbff8UL

/* UART16550A */
#define QEMU_VIRT_UART0_BASE  0x10000000UL
#define QEMU_VIRT_UART1_BASE  0x10000100UL

/* HTIF (Host Target Interface) for early console */
#define HTIF_BASE             0x40008000UL
#define HTIF_CONSOLE_DEV      1

/* Poweroff and reboot device */
#define VIRT_TEST_BASE        0x100000L
#define VIRT_TEST_FINISHER    0

/* PCI-E ECAM */
#define QEMU_VIRT_PCIE_ECAM   0x30000000UL
#define QEMU_VIRT_PCIE_MMIO   0x40000000UL

/* Clock frequency */
#define QEMU_VIRT_CLOCK_FREQ  10000000  /* 10MHz base clock */

/* Number of interrupts */
#define QEMU_VIRT_NR_IRQS     96

/* Device tree compatible strings */
#define QEMU_VIRT_COMPATIBLE  "riscv-virtio"

/* Board identification */
#define BOARD_NAME            "QEMU RISC-V Virt"
#define BOARD_REV             "1.0"

#endif /* _BOARD_QEMU_VIRT_H */