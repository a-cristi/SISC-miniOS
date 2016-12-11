#include "defs.h"
#include "ntstatus.h"
#include "log.h"
#include "dtr.h"
#include "panic.h"
#include "debugger.h"

//
// Exception codes
//
#define EX_CODE_DIVIDE_ERROR            0
#define EX_CODE_DEBUG                   1
#define EX_CODE_NMI                     2
#define EX_CODE_BREAKPOINT              3
#define EX_CODE_OVERFLOW                4
#define EX_CODE_BOUND                   5
#define EX_CODE_INVALID_OPCODE          6
#define EX_CODE_DEVICE_NOT_AVAIL        7
#define EX_CODE_DOUBLE_FAULT            8
#define EX_CODE_COPROC                  9
#define EX_CODE_INVALID_TSS             10
#define EX_CODE_SEGMENT_NOT_PRESENT     11
#define EX_CODE_STACK_FAULT             12
#define EX_CODE_GENERAL_PROTECTION      13
#define EX_CODE_PAGE_FAULT              14
#define EX_CODE_FPU_ERROR               16
#define EX_CODE_ALIGNMENT_CHECK         17
#define EX_CODE_MACHINE_CHECK           18
#define EX_CODE_SIMD_ERROR              19


//
// Keep in sync with hwexc.yasm! 
//
#pragma pack(push)
#pragma pack(1)

typedef struct _TRAP_FRAME
{
    QWORD       Home[4];

    QWORD       _Reserved0;

    PVOID       Self;

    QWORD       ExceptionCode;

    QWORD       RegRax;
    QWORD       RegRbx;
    QWORD       RegRdx;
    QWORD       RegRcx;
    QWORD       RegRsi;
    QWORD       RegRdi;
    QWORD       RegR8;
    QWORD       RegR9;
    QWORD       RegR10;
    QWORD       RegR11;
    QWORD       RegR12;
    QWORD       RegR13;
    QWORD       RegR14;
    QWORD       RegR15;

    WORD        SegDs;
    WORD        _DsFill[3];

    WORD        SegEs;
    WORD        _EsFill[3];

    WORD        SegFs;
    WORD        _FsFill[3];

    WORD        SegGs;
    WORD        _GsFill[3];

    QWORD       Cr2;
    QWORD       _Reserved1;

    QWORD       RegRbp;

    QWORD       ErrorCode;

    QWORD       RegRip;
    WORD        SegCs;
    WORD        _CsFill[3];

    QWORD       RegRsp;
    WORD        SegSs;
    WORD        _SsFill[3];
} TRAP_FRAME, *PTRAP_FRAME;

#pragma pack(pop)

// Check the size
static_assert(sizeof(TRAP_FRAME) == 0x0108, "Invalid TRAP_FRAME size");

// Check TRAP_FRAME.Self offset
static_assert(((QWORD)&(((TRAP_FRAME *)0)->Self)) == 5 * sizeof(QWORD), "TRAP_FRAME.Self at wrong offset");

//
// Import the ASM handlers (from hwexcp.yasm)
//
extern VOID ExHndDivideError(VOID);
extern VOID ExHndDbg(VOID);
extern VOID ExHndNMI(VOID);
extern VOID ExHndBp(VOID);
extern VOID ExHndOverflow(VOID);
extern VOID ExHndBnd(VOID);
extern VOID ExHndInvalidOpcode(VOID);
extern VOID ExHndDeviceNotAvailable(VOID);
extern VOID ExHndDoubleFault(VOID);
extern VOID ExHndCoprocSegmentOverrun(VOID);
extern VOID ExHndInvalidTSS(VOID);
extern VOID ExHndSegmentNotPresent(VOID);
extern VOID ExHndStackFault(VOID);
extern VOID ExHndGeneralProtection(VOID);
extern VOID ExHndPageFault(VOID);
extern VOID ExHndReserved15(VOID);
extern VOID ExHndFPUError(VOID);
extern VOID ExHndAlignmentCheck(VOID);
extern VOID ExHndMachineCheck(VOID);
extern VOID ExHndSIMDError(VOID);


static __inline
PCHAR
_ExCodeToString(
    _In_ DWORD Code
)
{
#define RETURN_IF_EQ(c, d)  if ((c) == (d)) { return #d; }

    RETURN_IF_EQ(Code, EX_CODE_DIVIDE_ERROR);
    RETURN_IF_EQ(Code, EX_CODE_DEBUG);
    RETURN_IF_EQ(Code, EX_CODE_NMI);
    RETURN_IF_EQ(Code, EX_CODE_BREAKPOINT);
    RETURN_IF_EQ(Code, EX_CODE_OVERFLOW);
    RETURN_IF_EQ(Code, EX_CODE_BOUND);
    RETURN_IF_EQ(Code, EX_CODE_INVALID_OPCODE);
    RETURN_IF_EQ(Code, EX_CODE_DEVICE_NOT_AVAIL);
    RETURN_IF_EQ(Code, EX_CODE_DOUBLE_FAULT);
    RETURN_IF_EQ(Code, EX_CODE_COPROC);
    RETURN_IF_EQ(Code, EX_CODE_INVALID_TSS);
    RETURN_IF_EQ(Code, EX_CODE_SEGMENT_NOT_PRESENT);
    RETURN_IF_EQ(Code, EX_CODE_STACK_FAULT);
    RETURN_IF_EQ(Code, EX_CODE_GENERAL_PROTECTION);
    RETURN_IF_EQ(Code, EX_CODE_PAGE_FAULT);
    RETURN_IF_EQ(Code, EX_CODE_FPU_ERROR);
    RETURN_IF_EQ(Code, EX_CODE_ALIGNMENT_CHECK);
    RETURN_IF_EQ(Code, EX_CODE_MACHINE_CHECK);
    RETURN_IF_EQ(Code, EX_CODE_SIMD_ERROR);
    return "Unknown";
}


static
VOID
_ExDumpExceptionInformation(
    _In_ DWORD Code,
    _In_ PTRAP_FRAME TrapFrame
)
{
    QWORD cr0 = __readcr0();
    QWORD cr2 = __readcr2();
    QWORD cr3 = __readcr3();
    QWORD cr4 = __readcr4();
    QWORD rflags = __readeflags();

    Log("\n!!======================================================================!!\n");
    Log("Exception: %d %s Trap frame @ %018p / %018p Error code: 0x%016llx\n", 
        Code, _ExCodeToString(Code), TrapFrame, TrapFrame->Self, TrapFrame->ErrorCode);
    Log("CR0 = 0x%016llx CR2 = 0x%016llx CR3 = 0x%016llx CR4 = 0x%016llx\n", cr0, cr2, cr3, cr4);
    Log("RFLAGS = 0x%016llx\n", rflags);
    Log("RAX: 0x%016llx RCX: 0x%016llx\n", TrapFrame->RegRax, TrapFrame->RegRcx);
    Log("RDX: 0x%016llx RBX: 0x%016llx\n", TrapFrame->RegRdx, TrapFrame->RegRbx);
    Log("RSP: 0x%016llx RBP: 0x%016llx\n", TrapFrame->RegRsp, TrapFrame->RegRbp);
    Log("RSI: 0x%016llx RDI: 0x%016llx\n", TrapFrame->RegRsi, TrapFrame->RegRdi);
    Log("R8 : 0x%016llx R9 : 0x%016llx\n", TrapFrame->RegR8, TrapFrame->RegR9);
    Log("R10: 0x%016llx R11: 0x%016llx\n", TrapFrame->RegR10, TrapFrame->RegR11);
    Log("R12: 0x%016llx R13: 0x%016llx\n", TrapFrame->RegR12, TrapFrame->RegR13);
    Log("R14: 0x%016llx R15: 0x%016llx\n", TrapFrame->RegR14, TrapFrame->RegR15);
    Log("RIP: 0x%016llx\n", TrapFrame->RegRip);
    Log("CS: 0x%04x DS: 0x%04x ES: 0x%04x FS: 0x%04x GS: 0x%04x SS: 0x%04x\n",
        TrapFrame->SegCs, TrapFrame->SegDs, TrapFrame->SegEs, TrapFrame->SegFs, TrapFrame->SegGs, TrapFrame->SegSs);

    if (EX_CODE_INVALID_OPCODE == Code)
    {
        // try to read 16 bytes from RIP
        Log("Code snippet at RIP:\n");
        PBYTE pCode = (BYTE *)TrapFrame->RegRip;
        for (BYTE i = 0; i < 16; i++)
        {
            Log("0x%02x ", pCode[i]);
        }
        Log("\n");
    }
}

//
// Common C handler, called by each of the above handlers
//
VOID
ExHndCommon(
    _In_ DWORD Code,
    _In_ PTRAP_FRAME TrapFrame
)
{
    _ExDumpExceptionInformation(Code, TrapFrame);
    PANIC(_ExCodeToString(Code));
}


static
VOID
_ExSetInterruptHandler(
    _Inout_ PINTERRUPT_GATE Idt,
    _In_ DWORD Vector,
    _In_ PVOID Handler,
    _In_ WORD Selector,
    _In_ WORD Attributes
)
{
    QWORD qwHandler = (QWORD)Handler;
    PINTERRUPT_GATE pGate = &Idt[Vector];

    memset(pGate, 0, sizeof(INTERRUPT_GATE));

    pGate->Offset_15_00 = qwHandler & 0xFFFF;
    pGate->Offset_31_16 = (qwHandler >> 16) & 0xFFFF;
    pGate->Offset_63_32 = (qwHandler >> 32) & 0xFFFFFFFF;

    pGate->Selector = Selector;
    pGate->Fields = Attributes;
    pGate->_Reserved = 0;
}


static
VOID
_ExFillIdt(
    _Inout_ INTERRUPT_GATE *Idt
)
{
    const QWORD handlers[20] =
    {
        /*  0 */ (QWORD)&ExHndDivideError,
        /*  1 */ (QWORD)&ExHndDbg,
        /*  2 */ (QWORD)&ExHndNMI,
        /*  3 */ (QWORD)&ExHndBp,
        /*  4 */ (QWORD)&ExHndOverflow,
        /*  5 */ (QWORD)&ExHndBnd,
        /*  6 */ (QWORD)&ExHndInvalidOpcode,
        /*  7 */ (QWORD)&ExHndDeviceNotAvailable,
        /*  8 */ (QWORD)&ExHndDoubleFault,
        /*  9 */ (QWORD)&ExHndCoprocSegmentOverrun,
        /* 10 */ (QWORD)&ExHndInvalidTSS,
        /* 11 */ (QWORD)&ExHndSegmentNotPresent,
        /* 12 */ (QWORD)&ExHndStackFault,
        /* 13 */ (QWORD)&ExHndGeneralProtection,
        /* 14 */ (QWORD)&ExHndPageFault,
        /* 15 */ (QWORD)NULL,
        ///* 15 */ (QWORD)&ExHndReserved15,
        /* 16 */ (QWORD)&ExHndFPUError,
        /* 17 */ (QWORD)&ExHndAlignmentCheck,
        /* 18 */ (QWORD)&ExHndMachineCheck,
        /* 19 */ (QWORD)&ExHndSIMDError,
    };

    memset(Idt, 0, sizeof(INTERRUPT_GATE) * IDT_ENTRIES);

    for (BYTE i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++)
    {
        _ExSetInterruptHandler(Idt, i, (PVOID)handlers[i], GDT_KCODE64_SELECTOR, 0x8E00);
    }
}


NTSTATUS
ExInitExceptionHandling(
    _Inout_ INTERRUPT_GATE *Idt
)
{
    if (!Idt)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    // add the exception handlers to the IDT
    _ExFillIdt(Idt);

    return STATUS_SUCCESS;
}
