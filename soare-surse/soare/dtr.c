#include "defs.h"
#include "ntstatus.h"
#include "dtr.h"
#include "kpool.h"
#include "kernel.h"
#include "log.h"

extern VOID IsrHndSpurious(VOID);

VOID HwLoadTr(_In_ WORD Selector);
VOID HwLoadCs(_In_ WORD Selector);
VOID HwLoadDs(_In_ WORD Selector);
VOID HwLoadEs(_In_ WORD Selector);
VOID HwLoadFs(_In_ WORD Selector);
VOID HwLoadGs(_In_ WORD Selector);
VOID HwLoadSs(_In_ WORD Selector);

WORD HwStoreTr(VOID);
WORD HwStoreCs(VOID);
WORD HwStoreDs(VOID);
WORD HwStoreEs(VOID);
WORD HwStoreFs(VOID);
WORD HwStoreGs(VOID);
WORD HwStoreSs(VOID);

NTSTATUS
ExInitExceptionHandling(
    _Inout_ INTERRUPT_GATE *Idt
);


static
VOID
_DtrInstallSpuriousHandlers(
    _In_ PPCPU Cpu
)
{
    BYTE spuriousVectors[] = { 0x27 };
    QWORD qwHandler = (QWORD)IsrHndSpurious;

    for (BYTE i = 0; i < sizeof(spuriousVectors) / sizeof(spuriousVectors[0]); i++)
    {
        PINTERRUPT_GATE pGate;
        pGate = &Cpu->Idt[spuriousVectors[i]];
        memset(pGate, 0, sizeof(INTERRUPT_GATE));
        pGate->Offset_15_00 = qwHandler & 0xFFFF;
        pGate->Offset_31_16 = (qwHandler >> 16) & 0xFFFF;
        pGate->Offset_63_32 = (qwHandler >> 32) & 0xFFFFFFFF;

        pGate->Selector = GDT_KCODE64_SELECTOR;
        pGate->Fields = 0x8E00;
    }
}


NTSTATUS
DtrInitAndLoadAll(
    _Inout_ PCPU *Cpu
)
{
    if (!Cpu)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    // prepare the TSS
    memset(&Cpu->Tss, 0, sizeof(Cpu->Tss));
    Cpu->Tss.IoMapBase = 0x68;  // ;)
    // no ISTs (yet) so everything else is 0

    // prepare the GDT
    memset(&Cpu->Gdt, 0, sizeof(Cpu->Gdt));
    Cpu->Gdt.NullDescriptor = GDT_NULL_DESCRIPTOR;
    Cpu->Gdt.KCode64 = GDT_KCODE64_DESCRIPTOR;
    Cpu->Gdt.KData64 = GDT_KDATA64_DESCRIPTOR;
    // Set the TSS descriptor
    Cpu->Gdt.Tss.MustBeZero = 0;
    Cpu->Gdt.Tss.Limit_15_00 = (sizeof(Cpu->Tss) - 1) & 0xFFFF;
    Cpu->Gdt.Tss.Limit_19_16 = ((sizeof(Cpu->Tss) - 1) >> 16) & 0xF;
    Cpu->Gdt.Tss.Base_15_00 = (QWORD)&Cpu->Tss & 0xFFFF;
    Cpu->Gdt.Tss.Base_23_16 = ((QWORD)&Cpu->Tss >> 16) & 0xFF;
    Cpu->Gdt.Tss.Base_31_24 = ((QWORD)&Cpu->Tss >> 24) & 0xFF;
    Cpu->Gdt.Tss.Base_63_32 = ((QWORD)&Cpu->Tss >> 32) & 0xFFFFFFFF;
    Cpu->Gdt.Tss.Fields = 0x89; // Present, DPL = 0

    // prepare the IDT
    ExInitExceptionHandling(Cpu->Idt);

    // install a dummy handler for the spurious IRQs
    _DtrInstallSpuriousHandlers(Cpu);

    // setup GDTR
    Cpu->Gdtr.Address = (QWORD)&Cpu->Gdt;
    Cpu->Gdtr.Size = sizeof(Cpu->Gdt) - 1;

    // setup TR
    Cpu->Tr.Address = (QWORD)&Cpu->Tss;
    Cpu->Tr.Size = sizeof(Cpu->Tss) - 1;

    // setup IDTR
    Cpu->Idtr.Address = (QWORD)&Cpu->Idt;
    Cpu->Idtr.Size = sizeof(Cpu->Idt) - 1;

    // load the GDT
    LogWithInfo("[%d] Loading the GDT...\n", Cpu->ApicId);
    _lgdt(&Cpu->Gdtr);
    LogWithInfo("[%d] Done!\n", Cpu->ApicId);

    // load the IDT
    LogWithInfo("[%d] Loading the IDT...\n", Cpu->ApicId);
    __lidt(&Cpu->Idtr);
    LogWithInfo("[%d] Done!\n", Cpu->ApicId);

    // load the TR
    LogWithInfo("[%d] Loading the TR...\n", Cpu->ApicId);
    HwLoadTr(GDT_TSS_SELECTOR);
    LogWithInfo("[%d] Done!\n", Cpu->ApicId);

    // Load Selectors
    LogWithInfo("[%d] Loading CS...\n", Cpu->ApicId);
    HwLoadCs(GDT_KCODE64_SELECTOR);
    LogWithInfo("[%d] Done!\n", Cpu->ApicId);

    LogWithInfo("[%d] Loading DS...\n", Cpu->ApicId);
    HwLoadDs(GDT_KDATA64_SELECTOR);
    LogWithInfo("[%d] Done!\n", Cpu->ApicId);
    
    LogWithInfo("[%d] Loading ES...\n", Cpu->ApicId);
    HwLoadEs(GDT_KDATA64_SELECTOR);
    LogWithInfo("[%d] Done!\n", Cpu->ApicId);

    LogWithInfo("[%d] Loading FS...\n", Cpu->ApicId);
    HwLoadFs(GDT_KDATA64_SELECTOR);
    LogWithInfo("[%d] Done!\n", Cpu->ApicId);

    LogWithInfo("[%d] Loading GS...\n", Cpu->ApicId);
    HwLoadGs(GDT_KDATA64_SELECTOR);
    LogWithInfo("[%d] Done!\n", Cpu->ApicId);

    LogWithInfo("[%d] Loading SS...\n", Cpu->ApicId);
    HwLoadSs(GDT_KDATA64_SELECTOR);
    LogWithInfo("[%d] Done!\n", Cpu->ApicId);

    // Set the PCPU
    __writemsr(IA32_GS_BASE, Cpu);
    __writemsr(IA32_KERNEL_GS_BASE, Cpu);

    return STATUS_SUCCESS;
}


static PCPU gBsp;

NTSTATUS
DtrCreatePcpu(
    _Out_ PPCPU *Cpu
)
{
    if (!Cpu)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    *Cpu = &gBsp;
    return STATUS_SUCCESS;
}


NTSTATUS
DtrInstallIrqHandler(
    _In_ WORD Index,
    _In_ PFN_IrqHandler Handler
)
{
    QWORD qwHandler = (QWORD)Handler;
    PPCPU pCpu = GetCurrentCpu();
    PINTERRUPT_GATE pGate;

    if (Index >= IDT_ENTRIES)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!Handler)
    {
        return STATUS_INVALID_PARAMETER_4;
    }


    pGate = &pCpu->Idt[Index];
    LogWithInfo("[IDT] Installing %018p as the handler for IRQ 0x%04x [%018p:%018p]...\n", 
        qwHandler, Index, pCpu->Idtr.Address, pGate);
    memset(pGate, 0, sizeof(INTERRUPT_GATE));
    pGate->Offset_15_00 = qwHandler & 0xFFFF;
    pGate->Offset_31_16 = (qwHandler >> 16) & 0xFFFF;
    pGate->Offset_63_32 = (qwHandler >> 32) & 0xFFFFFFFF;

    pGate->Selector = GDT_KCODE64_SELECTOR;
    pGate->Fields = 0x8E00;
    LogWithInfo("DPL %d IST %d P: %d S %d Tpe: 0x%04x\n", pGate->DPL, pGate->Ist, pGate->P, pGate->S, pGate->Type);
    __lidt(&pCpu->Idtr);

    return STATUS_SUCCESS;
}
