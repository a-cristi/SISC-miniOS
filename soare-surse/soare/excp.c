#include "defs.h"
#include "ntstatus.h"
#include "log.h"
#include "dtr.h"
#include "panic.h"
#include "debugger.h"

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

static_assert(sizeof(TRAP_FRAME) == 0x0108, "Invalid TRAP_FRAME size");

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

//
// Common C handler, called by each of the above handlers
//
VOID
ExHndCommon(
    _In_ DWORD Code,
    _In_ PTRAP_FRAME TrapFrame
)
{
    LogWithInfo("[FATAL] Exception 0x%08x at RIP %018p!\n", Code, TrapFrame->RegRip);

    PANIC("EXCEPTION");
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
    const QWORD handlers[21] =
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
        /* 16 */ (QWORD)&ExHndReserved15,
        /* 17 */ (QWORD)&ExHndFPUError,
        /* 18 */ (QWORD)&ExHndAlignmentCheck,
        /* 19 */ (QWORD)&ExHndMachineCheck,
        /* 20 */ (QWORD)&ExHndSIMDError,
    };

    memset(Idt, 0, sizeof(INTERRUPT_GATE) * IDT_ENTRIES);

    for (BYTE i = 0; i < 22; i++)
    {
        _ExSetInterruptHandler(Idt, i, (PVOID)handlers[i], GDT_CODEK_SELECTOR, 0x8E00);
    }
}


static INTERRUPT_GATE gIdt[IDT_ENTRIES];
static IDTR gIdtr;

NTSTATUS
ExInitExceptionHandling(
    _Inout_ INTERRUPT_GATE *Idt,
    _Inout_ IDTR *Idtr
)
{
    if (!Idt)
    {
        Idt = gIdt;
        //return STATUS_INVALID_PARAMETER_1;
    }

    if (!Idtr)
    {
        Idtr = &gIdtr;
    }

    // add the exception handlers to the IDT
    _ExFillIdt(Idt);

    // load the IDT
    Idtr->Address = (QWORD)Idt;
    Idtr->Size = (IDT_ENTRIES * sizeof(INTERRUPT_GATE)) - 1;
    __lidt(Idtr);

    return STATUS_SUCCESS;
}
