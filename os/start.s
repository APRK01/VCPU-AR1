.global _start

_start:
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    ldr x1, =0x44000000
    mov x2, #0x10000
    mul x2, x2, x0
    sub x1, x1, x2
    mov sp, x1
    
    ldr x0, =vectors
    msr vbar_el1, x0
    
    msr SPSel, #1
    
    bl kernel_main
    
hang:
    wfi
    b hang

.align 11
vectors:
    /* Current EL with SP0 */
    .align 7
    b hang          /* Sync */
    .align 7
    b irq_handler   /* IRQ */
    .align 7
    b hang          /* FIQ */
    .align 7
    b hang          /* SError */

    /* Current EL with SPx */
    .align 7
    b hang          /* Sync */
    .align 7
    b irq_handler   /* IRQ */
    .align 7
    b hang          /* FIQ */
    .align 7
    b hang          /* SError */

    /* Lower EL using AArch64 */
    .align 7
    b hang
    .align 7
    b hang
    .align 7
    b hang
    .align 7
    b hang

    /* Lower EL using AArch32 */
    .align 7
    b hang
    .align 7
    b hang
    .align 7
    b hang
    .align 7
    b hang

irq_handler:
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x18, x30, [sp, #-16]!
    
    bl c_irq_handler
    
    ldp x18, x30, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x8, x9, [sp], #16
    ldp x6, x7, [sp], #16
    ldp x4, x5, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
    eret
