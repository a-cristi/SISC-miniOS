[bits 64]

; Exception codes
EX_DIVIDE_ERROR         equ 00
EX_DEBUG                equ 01
EX_NMI                  equ 02
EX_BREAKPOINT           equ 03
EX_OVERFLOW             equ 04
EX_BOUND                equ 05
EX_INVALID_OPCODE       equ 06
EX_DEVICE_NOT_AVAILABLE equ 07
EX_DOUBLE_FAULT         equ 08
EX_COPROC_SEG_OVERRUN   equ 09
EX_INVALID_TSS          equ 10
EX_SEG_NOT_PRESENT      equ 11
EX_STACK_FAULT          equ 12
EX_GENERAL_PROTECTION   equ 13
EX_PAGE_FAULT           equ 14
; 15 is reserved
EX_FPU_ERROR            equ 16
EX_ALIGNMENT_CHECK      equ 17
EX_MACHINE_CHECK        equ 18
EX_SIMD                 equ 19
EX_VE                   equ 20


STRUC EX_FRAME
    .P1                 resq 1
    .P2                 resq 1
    .P3                 resq 1
    .P4                 resq 1
    .Reserved           resq 1

    .Self               resq 1

    .ExceptionCode      resq 1

    ; GPRs
    .RegRax             resq 1
    .RegRbx             resq 1
    .RegRdx             resq 1
    .RegRcx             resq 1
    .RegRsi             resq 1
    .RegRdi             resq 1
    .RegR8              resq 1
    .RegR9              resq 1
    .RegR10             resq 1
    .RegR11             resq 1
    .RegR12             resq 1
    .RegR13             resq 1
    .RegR14             resq 1
    .RegR15             resq 1

    ; Segment registers
    .SegDs              resw 1
    ._SegDsFillW        resw 1
    ._SegDsFillD        resd 1

    .SegEs              resw 1
    ._SegEsFillW        resw 1
    ._SegEsFillD        resd 1

    .SegFs              resw 1
    ._SegFsFillW        resw 1
    ._SegFsFillD        resd 1

    .SegGs              resw 1
    ._SegGsFillW        resw 1
    ._SegGsFillD        resd 1

    ; CR2 (valid only on #PF)
    .RegCr2             resq 1

    .Reserved2          resq 1

    ; error code pushed by the CPU (not always valid)
    .PushedErrorCode    resq 1

    ; the RIP at which the exception was generated
    .RegRip             resq 1
    .SegCs              resw 1
    ._SegCsFillW        resw 1
    ._SegCsFillD        resd 1

    .RegRflags          resq 1

    ; Stack
    .RegRsp             resq 1
    .SegSs              resw 1
    ._SegSsFillW        resw 1
    ._SegSsFillD        resd 1
ENDSTRUC

; frame generators
%macro GENERATE_EX_FRAME 1
    %if (1 != %1)
        push    rbp
    %endif

    push    rbp

    sub     rsp, (EX_FRAME_size - 7 * 8)
    mov     rbp, rsp

    mov     [rbp + EX_FRAME.RegRax], rax
    mov     [rbp + EX_FRAME.RegRbx], rbx
    mov     [rbp + EX_FRAME.RegRcx], rcx
    mov     [rbp + EX_FRAME.RegRdx], rdx
    mov     [rbp + EX_FRAME.RegRsi], rsi
    mov     [rbp + EX_FRAME.RegRdi], rdi
    mov     [rbp + EX_FRAME.RegR8],  r8
    mov     [rbp + EX_FRAME.RegR9],  r9
    mov     [rbp + EX_FRAME.RegR10], r10
    mov     [rbp + EX_FRAME.RegR11], r11
    mov     [rbp + EX_FRAME.RegR12], r12
    mov     [rbp + EX_FRAME.RegR13], r13
    mov     [rbp + EX_FRAME.RegR14], r14
    mov     [rbp + EX_FRAME.RegR15], r15

    mov     ax, ds
    mov     [rbp + EX_FRAME.SegDs], ax

    mov     ax, es
    mov     [rbp + EX_FRAME.SegEs], ax

    mov     ax, fs
    mov     [rbp + EX_FRAME.SegFs], ax

    mov     ax, gs
    mov     [rbp + EX_FRAME.SegGs], ax
%endmacro

%macro EXCEPTION_HANDLER %2
    GENERATE_EX_FRAME %1

    mov     rcx, %2
    mov     [rbp + EX_FRAME.ExceptionCode], rcx
    mov     rds, rbp

    cli
    hlt
%endmacro

; actual handlers
ExDivideError:                  EXCEPTION_HANDLER(0, EX_DIVIDE_ERROR)
ExDebug:                        EXCEPTION_HANDLER(0, EX_DEBUG)
ExNmi:                          EXCEPTION_HANDLER(0, EX_NMI)
ExBreakpoint:                   EXCEPTION_HANDLER(0, EX_BREAKPOINT)
ExOverflow:                     EXCEPTION_HANDLER(0, EX_OVERFLOW)
ExBound:                        EXCEPTION_HANDLER(0, EX_BOUND)
ExInvalidOpcode:                EXCEPTION_HANDLER(0, EX_INVALID_OPCODE)
ExDeviceNotAvailable:           EXCEPTION_HANDLER(0, EX_DEVICE_NOT_AVAILABLE)
ExDoubleFault:                  EXCEPTION_HANDLER(1, EX_DOUBLE_FAULT)
ExCoprocessorSegmentOverrun:    EXCEPTION_HANDLER(0, EX_COPROC_SEG_OVERRUN)
ExInvalidTss:                   EXCEPTION_HANDLER(1, EX_INVALID_TSS)
ExSegmentNotPresent:            EXCEPTION_HANDLER(1, EX_SEG_NOT_PRESENT)
ExStackFault:                   EXCEPTION_HANDLER(1, EX_STACK_FAULT)
ExGeneralProtection:            EXCEPTION_HANDLER(1, EX_GENERAL_PROTECTION)
ExPageFault:                    EXCEPTION_HANDLER(1, EX_PAGE_FAULT)
ExFpuError:                     EXCEPTION_HANDLER(0, EX_FPU_ERROR)
ExAlignmentCheck:               EXCEPTION_HANDLER(1, EX_ALIGNMENT_CHECK)
ExMachineCheck:                 EXCEPTION_HANDLER(0, EX_MACHINE_CHECK)
ExSimd:                         EXCEPTION_HANDLER(0, EX_SIMD)