/* Early console output for QEMU virt */
/* Using simple direct write without initialization */

.section .text
.globl early_putchar
.globl early_puts
.globl early_uart_init

/* UART base address for QEMU virt (NS16550A with 32-bit registers) */
.equ UART_BASE, 0x10000000
.equ UART_THR, 0x00    /* Transmitter Holding Register */
.equ UART_IER, 0x04    /* Interrupt Enable Register */
.equ UART_FCR, 0x08    /* FIFO Control Register */
.equ UART_LCR, 0x0C    /* Line Control Register */
.equ UART_MCR, 0x10    /* Modem Control Register */
.equ UART_LSR, 0x14    /* Line Status Register */
.equ UART_DLL, 0x00    /* Divisor Latch Low (when DLAB=1) */
.equ UART_DLH, 0x04    /* Divisor Latch High (when DLAB=1) */

/* Initialize UART - do nothing, QEMU doesn't need it */
early_uart_init:
    ret

/* Put character - simplified version without delay */
early_putchar:
    li t4, UART_BASE
    sw a0, UART_THR(t4)
    ret

/* Put string - simple direct write */
early_puts:
    li t0, UART_BASE
1:
    lbu t1, 0(a0)      /* Load character from string */
    beqz t1, 2f        /* If null, done */

    sw t1, UART_THR(t0) /* Write directly to UART */

    /* Small delay loop */
    li t2, 100
3:
    addi t2, t2, -1
    bnez t2, 3b

    addi a0, a0, 1      /* Next character */
    j 1b
2:
    ret
