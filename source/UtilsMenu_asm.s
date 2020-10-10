nsub_020A2210:
    push {r0-r12, r14}
    mov r1, #0
    mov r0, #238
    bl PlaySNDEffect
    pop {r0-r12, r14}
    cmp r0, r0
    beq 0x020A21C4
    b 0x020A2238