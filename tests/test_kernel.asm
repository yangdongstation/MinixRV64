# Minimal RISC-V test kernel that prints "Hello RISC-V!"
.section .text
.globl _start
_start:
    la a0, message
    li a7, 64  # Linux write syscall for demo (will be replaced)
    ecall

    # Infinite loop
loop:
    wfi
    j loop

message:
    .string "Hello Minix RV64!\n"
