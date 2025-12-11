# Current Debugging Status - MinixRV64 Input Issue

## 🎯 Problem Summary

The shell cannot receive input from the UART after MMU is enabled.

## 🔍 What We've Discovered

### Execution Flow Analysis

Through progressive debugging, we've traced exactly where the system hangs:

```
Boot sequence:
✓ MMU init (pgtable_init)
✓ "[MMU] Mapping MMIO devices..." appears
✓ "[MMU] MMIO mapping complete" appears
✓ MMU enabled (enable_mmu)
✓ Shell initialized
✓ "minix# " prompt printed
✓ "[LOOP]" marker appears (shell_run while loop starts)
✓ "1" marker appears (first loop iteration begins)
✓ "G" marker appears (uart_getc() function entered)
✗ HANGS - never prints "L" marker (which is after uart_read_reg(UART_LSR))
```

###  Precise Hang Location

**File**: drivers/char/uart.c
**Function**: `uart_getc()`
**Line**: `volatile unsigned int lsr = uart_read_reg(BOARD_UART_BASE, UART_LSR);`

The system hangs when trying to read the UART Line Status Register at physical address `0x10000014` (BOARD_UART_BASE + UART_LSR offset).

## 🗺️ MMU Mapping Configuration

### Current Page Table Setup (arch/riscv64/mm/pgtable.c)

```c
/* Kernel code/data mapping */
0x80000000 - 0x81000000: Identity mapped, R/W/X permissions

/* MMIO device mapping */
0x00000000 - 0x80000000: Identity mapped, R/W permissions (no X)
```

This **should** include:
- 0x02000000: CLINT
- 0x0c000000: PLIC
- 0x10000000: UART ← **Our target**
- 0x40008000: HTIF

### Mapping Implementation

```c
va = 0x00000000UL;
end_va = 0x80000000UL;

while (va < end_va) {
    /* 1GB gigapage mapping */
    pte_t *pte = &root_pgtable[va >> PGDIR_SHIFT];
    *pte = (va >> 12) << PTE_PPN_SHIFT;
    *pte |= PTE_V | PTE_R | PTE_W;
    va += (1UL << PGDIR_SHIFT);  /* Increment by 1GB */
}
```

## ❓ Why Is It Still Hanging?

Despite the MMIO mapping code:
1. The mapping code **is executing** (we see the debug messages)
2. The page table entries **are being set**
3. But accessing 0x10000000 **still hangs**

### Possible Causes

1. **Page Table Entry Format Issue**
   - RISC-V SV39 gigapage format may not be correct
   - PPN field calculation might be wrong for gigapages
   - Leaf vs non-leaf PTE distinction

2. **TLB Not Flushed Properly**
   - `sfence.vma` after enable_mmu() might not be sufficient
   - Need per-page TLB flush?

3. **SATP Configuration**
   - SATP value calculation might be incorrect
   - Mode bits (SV39 = 8) in wrong position

4. **Virtual vs Physical Address Confusion**
   - After MMU enabled, are we using correct addresses?
   - Is BOARD_UART_BASE defined correctly for post-MMU use?

5. **Page Table Walker Issue**
   - Hardware page table walker might not be finding the mapping
   - Root page table physical address wrong?

## 📊 Mapping Calculations

For UART at 0x10000000:

```
PGDIR_SHIFT = 30 (1GB pages)
Page table index = 0x10000000 >> 30 = 0

So UART should be in root_pgtable[0], which maps:
  Virtual: 0x00000000 - 0x3FFFFFFF
  Physical: 0x00000000 - 0x3FFFFFFF (identity)
```

This calculation seems correct.

## 🔧 Next Steps to Try

### 1. Verify Page Table Entry Contents

Add code to dump the PTE for address 0x10000000 and verify:
- V (Valid) bit is set
- R/W bits are set
- PPN contains correct physical page number

### 2. Check SATP Value

Print the SATP register value and verify:
- Mode field = 8 (SV39)
- PPN field = physical address of root_pgtable >> 12

### 3. Try 2MB Pages Instead of 1GB Pages

Use PMD level instead of PGD level for finer-grained mapping.

### 4. Check for Exception/Trap

The hang might actually be an infinite trap loop. Check if we're taking page faults repeatedly.

### 5. Simplify MMU Mapping

Try mapping ONLY 0x10000000-0x10001000 (single 4KB page) to isolate the issue.

## 📝 Code Markers for Debugging

Current debug markers in output:
- `X123`: Boot stages
- `[MMU] Mapping MMIO devices...`: MMU mapping started
- `[MMU] MMIO mapping complete`: MMU mapping finished
- `✓ MMU ready`: MMU enabled successfully
- `[LOOP]`: Shell main loop started
- `1`: First loop iteration
- `G`: uart_getc() entered
- `L`: (NOT appearing) After UART LSR read - **HANG POINT**

## 🎓 Technical Background

### RISC-V SV39 Paging

- 3-level page table
- 39-bit virtual addresses
- Support for 4KB, 2MB (megapage), 1GB (gigapage)
- PTE format:
  - Bits [53:10]: PPN (Physical Page Number)
  - Bits [7:0]: Flags (D,A,G,U,X,W,R,V)

### Gigapage PTEs

For a 1GB gigapage at the root level:
- Must be a leaf PTE (R,W,X bits determine this)
- PPN[2] corresponds to bits [38:30] of physical address
- PPN[1:0] should be zero for naturally aligned gigapages

## 🚨 Critical Question

**Is our gigapage PTE format correct for RISC-V?**

Current code:
```c
*pte = (va >> 12) << PTE_PPN_SHIFT;  // PPN from VA
*pte |= PTE_V | PTE_R | PTE_W;        // Valid, Read, Write
```

For va = 0x10000000:
- va >> 12 = 0x10000
- 0x10000 << 10 = 0x4000000 (bits [26:10])

But for a gigapage, we need the physical address bits [38:30] in PPN[2]...

**This might be the bug!**

---

**Status**: UART MMU mapping not working despite mapping code executing.
**Next**: Need to fix gigapage PTE format or switch to smaller pages.
