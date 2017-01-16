#include "defs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "dtr.h"
#include "virtmemmgr.h"
#include "smp.h"
#include "log.h"
#include "apic.h"

extern QWORD AP_START;
extern QWORD AP_END;

extern DWORD ApCpuCount;
extern DWORD ApInitLock;
extern BYTE ApFlags;
extern QWORD ApGdtAddress;
extern QWORD ApGdt64Address;
extern DWORD ApIdentityPdbr;
extern QWORD ApFinalPdbr;
extern QWORD ApEntryPoint;
extern BYTE ApGdt;
extern BYTE ApGdt64;
//extern DWORD ApProtectedModeJmpOffset;
extern DWORD ApProtectedMode;
extern BYTE ApSwitchToPm;


#define AP_BASE     0x7000


VOID
ApEntryPoint64(
    _In_ DWORD Count
)
{
    UNREFERENCED_PARAMETER(Count);
    while (TRUE);
}

#include "msrdefs.h"

NTSTATUS
MpPrepareApZone(
    _In_ PCPU_STATE CpuState
)
{
    DWORD apNeededSize = (DWORD)(&AP_END - &AP_START);
    PVOID pApBase = NULL;
    NTSTATUS status;
    PDWORD pApCpuCount = &ApCpuCount;
    PDWORD pApInitLock = &ApInitLock;
    PQWORD pApGdtAddr = &ApGdtAddress;
    PDWORD pApIdentityPdbr = &ApIdentityPdbr;
    PQWORD pApFinalPdbr = &ApFinalPdbr;
    PQWORD pApGdt64Addr = &ApGdt64Address;
    PQWORD pApEp = &ApEntryPoint;

    if (!CpuState)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    LogWithInfo("Needed size: %018p\n", apNeededSize);

    // set the variables
    *pApCpuCount = 1;
    *pApInitLock = 0;
    *pApGdtAddr = (SIZE_T)&ApGdt - (SIZE_T)&AP_START + AP_BASE;
    *pApGdt64Addr = (SIZE_T)&ApGdt64 - (SIZE_T)&AP_START + AP_BASE;
    *pApIdentityPdbr = 0x2000 + 0x200000;
    *pApFinalPdbr = CLEAN_PHYADDR(__readcr3());
    *pApEp = (QWORD)&ApEntryPoint64;

    {
        PBYTE p = &ApSwitchToPm;
        for (BYTE i = 0; i < 32; i++)
        {
            Log("%02x", p[i]);
        }
        Log("\n");

        Log("APGDTADDR: %018p\n", *pApGdtAddr);
    }

    status = MmMapPhysicalPages(AP_BASE, apNeededSize, &pApBase, MAP_FLG_SKIP_PHYPAGE_CHECK);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] MmMapPhysicalPages failed: 0x%08x\n", status);
        return status;
    }

    // copy the AP zone
    memcpy(pApBase, &AP_START, apNeededSize);

    for (BYTE i = 0; i < CpuState->CpuCount; i++)
    {
        PPCPU pPcpu = &CpuState->Cpus[i];
        DWORD icrHigh;
        DWORD icrLow;
        DWORD before = *pApCpuCount;

        if (pPcpu->IsBsp)
        {
            continue;
        }

        // Init
        icrHigh = 0 | (pPcpu->ApicId << 24);
        icrLow = 0 | ICR_LV_BIT | (ICR_DMODE_INIT << 8);

        LogWithInfo("ICR LOW: 0x%08x HIGH: 0x%08x\n", icrLow, icrHigh);

        ApicWriteRegister(LAPIC_REG_ICR_HIGH, icrHigh);
        ApicWriteRegister(LAPIC_REG_ICR_LOW, icrLow);

        while (0 != (ICR_DELIVERY_STS_BIT & ApicReadRegister(LAPIC_REG_ICR_LOW)));

        // 1st SIPI
        icrHigh = 0 | (pPcpu->ApicId << 24);
        icrLow = 0 | 0x4000 | (0x06 << 8) | (AP_BASE >> 12);

        LogWithInfo("ICR LOW: 0x%08x HIGH: 0x%08x\n", icrLow, icrHigh);

        ApicWriteRegister(LAPIC_REG_ICR_HIGH, icrHigh);
        ApicWriteRegister(LAPIC_REG_ICR_LOW, icrLow);

        while (0 != (ICR_DELIVERY_STS_BIT & ApicReadRegister(LAPIC_REG_ICR_LOW)));

        if (before != *pApCpuCount)
        {
            goto _ap_started;
        }

        // 2nd SIPI
        icrHigh = 0 | (pPcpu->ApicId << 24);
        icrLow = 0 | ICR_LV_BIT | (ICR_DMODE_SIPI << 8) | (AP_BASE >> 12);

        LogWithInfo("ICR LOW: 0x%08x HIGH: 0x%08x\n", icrLow, icrHigh);

        ApicWriteRegister(LAPIC_REG_ICR_HIGH, icrHigh);
        ApicWriteRegister(LAPIC_REG_ICR_LOW, icrLow);

        while (0 != (ICR_DELIVERY_STS_BIT & ApicReadRegister(LAPIC_REG_ICR_LOW)));

        if (before != *pApCpuCount)
        {
            goto _ap_started;
        }

        while (&i);
        Log("[AP] Failed to start %d\n", pPcpu->ApicId);
        break;

    _ap_started:
        while (&i);
        Log("[AP] Started %d\n", pPcpu->ApicId);

        break;
    }

    MmUnmapRangeAndNull(&pApBase, apNeededSize, MAP_FLG_SKIP_PHYPAGE_CHECK);

    return STATUS_SUCCESS;
}
