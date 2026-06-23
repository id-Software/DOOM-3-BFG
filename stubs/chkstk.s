    .text
    .globl __chkstk
__chkstk:
    pushq   %rcx
    movl    $0x10101010, %eax
    call    ___chkstk_ms
    popq    %rcx
    ret
