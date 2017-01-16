#include "defs.h"
#include "msrdefs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "log.h"
#include "apic.h"
#include "virtmemmgr.h"


static volatile PVOID gApicBase;


#define APIC_BASE_MASK      0xFFE00000

NTSTATUS
ApicInit(
    VOID
)
{
    QWORD apicBase = __readmsr(IA32_APIC_BASE_MSR);
    NTSTATUS status;
    PVOID pBase = NULL;

    Log("[APIC] IA32_APIC_BASE_MSR = %018p\n", apicBase);

    status = MmMapPhysicalPages(apicBase & APIC_BASE_MASK, PAGE_SIZE_4K, &pBase, MAP_FLG_SKIP_PHYPAGE_CHECK);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] MmMapPhysicalPages failed: 0x%08x\n", status);
        return status;
    }
    gApicBase = pBase;

    Log("[APIC] APIC @ %018p mapped @ %018p\n", apicBase & APIC_BASE_MASK, gApicBase);

    return STATUS_SUCCESS;
}


DWORD
ApicReadRegister(
    _In_ DWORD Register
)
{
    volatile PDWORD pReg = (DWORD  *)((SIZE_T)gApicBase + Register);

    return *pReg;
}


VOID
ApicWriteRegister(
    _In_ DWORD Register,
    _In_ DWORD Value
)
{
    volatile PDWORD pReg = (DWORD  *)((SIZE_T)gApicBase + Register);

    *pReg = Value;
}
