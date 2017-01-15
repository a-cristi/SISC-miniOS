#include "defs.h"
#include "ntstatus.h"
#include "varargs.h"
#include "log.h"
#include "string.h"
#include "acpitables.h"
#include "virtmemmgr.h"

/// TODO: integrate ACPICA lib to make this simpler

#define EBDA_PTR_PA_LOCATION        0x0000040E
#define EBDA_PTR_SIZE               2
#define EBDA_RANGE                  1024
#define RSDP_BASE_PA                0x000E0000
#define RSDP_RANGE                  0x00020000
#define RSDP_INCREMENT              16

#define RSDP_STD_CHECKSUM_SIZE      20
#define RSDP_EXT_CHECKSUM_SIZE      36


UINT8
AcpiGetTableChecksum(
    _In_ PBYTE Buffer,
    _In_ DWORD Length
)
{
    BYTE chks = 0;

    for (DWORD i = 0; i < Length; i++)
    {
        chks += Buffer[i];
    }

    return chks;
}


NTSTATUS
AcpiCheckRsdp(
    _In_ PRSDP_TABLE Rsdp
)
{
    if (strncmp(Rsdp->Signature, ACPI_RSDP_SIGNATURE, strlen(ACPI_RSDP_SIGNATURE)))
    {
        return STATUS_SIGNATURE_NOT_MATCHED;
    }

    if (AcpiGetTableChecksum((BYTE *)Rsdp, RSDP_STD_CHECKSUM_SIZE))
    {
        return STATUS_INVALID_CHECKSUM;
    }

    if (Rsdp->Revision >= 2 && AcpiGetTableChecksum((BYTE *)Rsdp, RSDP_EXT_CHECKSUM_SIZE))
    {
        return STATUS_INVALID_CHECKSUM;
    }

    return STATUS_SUCCESS;
}


PBYTE
AcpiSearchRsdp(
    _In_ PBYTE Start,
    _In_ DWORD Length
)
{
    NTSTATUS status;
    PBYTE pEnd = Start + Length;

    pEnd = Start + Length;

    for (PBYTE pIterator = Start; pIterator < pEnd; pIterator += RSDP_INCREMENT)
    {
        status = AcpiCheckRsdp((RSDP_TABLE *)pIterator);
        if (NT_SUCCESS(status))
        {
            return pIterator;
        }
    }

    return NULL;
}



NTSTATUS
AcpiFindRootPointer(
    _Out_ QWORD *TableAddress
)
{
    PBYTE pTable;
    PBYTE pIterator;
    DWORD paTable;
    NTSTATUS status;

    status = MmMapPhysicalPages(EBDA_PTR_PA_LOCATION, EBDA_PTR_SIZE, &pTable, MAP_FLG_SKIP_PHYPAGE_CHECK);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] MmMapPhysicalPages failed for %018p:0x%08x: 0x%08x\n", 
            EBDA_PTR_PA_LOCATION, EBDA_PTR_SIZE, status);
        return status;
    }

    paTable = 0;
    paTable = *(WORD *)(pTable);
    paTable <<= 4;

    MmUnmapRangeAndNull(&pTable, EBDA_PTR_SIZE, MAP_FLG_SKIP_PHYPAGE_CHECK);

    if (paTable > 0x400)
    {
        status = MmMapPhysicalPages(paTable, EBDA_RANGE, &pTable, MAP_FLG_SKIP_PHYPAGE_CHECK);
        if (!NT_SUCCESS(status))
        {
            LogWithInfo("[ERROR] MmMapPhysicalPages failed for %018p:0x%08x: 0x%08x\n",
                paTable, EBDA_RANGE, status);
            return status;
        }

        pIterator = AcpiSearchRsdp(pTable, EBDA_RANGE);
        MmUnmapRangeAndNull(&pTable, EBDA_RANGE, MAP_FLG_SKIP_PHYPAGE_CHECK);

        if (pIterator)
        {
            paTable += (DWORD)((SIZE_T)pIterator - (SIZE_T)pTable);

            *TableAddress = paTable;
            return STATUS_SUCCESS;
        }
    }

    status = MmMapPhysicalPages(RSDP_BASE_PA, RSDP_RANGE, &pTable, MAP_FLG_SKIP_PHYPAGE_CHECK);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] MmMapPhysicalPages failed for %018p:0x%08x: 0x%08x\n",
            RSDP_BASE_PA, RSDP_RANGE, status);
        return status;
    }

    pIterator = AcpiSearchRsdp(pTable, RSDP_RANGE);    
    MmUnmapRangeAndNull(&pTable, RSDP_RANGE, MAP_FLG_SKIP_PHYPAGE_CHECK);

    if (pIterator)
    {
        paTable = RSDP_BASE_PA + (DWORD)((SIZE_T)pIterator - (SIZE_T)pTable);

        *TableAddress = paTable;
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

VOID
AcpiDumpRsdp(
    _In_ PRSDP_TABLE Rsdp
)
{
    if (Rsdp)
    {
        CHAR signature[9] = { 0 };
        CHAR oemId[ACPI_OEM_ID_SIZE + 1] = { 0 };

        memcpy(signature, Rsdp->Signature, 8 * sizeof(CHAR));
        memcpy(oemId, Rsdp->OemId, ACPI_OEM_ID_SIZE);

        Log("[ACPI] RSDP: \n");
        Log("\t\t Signature: %s\n", signature);
        Log("\t\t Checksum:  0x%x\n", Rsdp->Checksum);
        Log("\t\t Oem ID:    %s\n", Rsdp->OemId);
        Log("\t\t Revision:  %d - %s\n", Rsdp->Revision,
            0 == Rsdp->Revision ? "ACPI 1.0" : 2 <= Rsdp->Revision ? "ACPI 2.0+" : "Unknown");
        Log("\t\t RSDT PA:   0x%x\n", Rsdp->RsdtPhysicalAddress);
        Log("\t\t Length:    0x%x\n", Rsdp->Length);
        Log("\t\t XSDT PA:   %p\n", Rsdp->XsdtPhysicalAddress);
        Log("\t\t ExChkSum:  0x%x\n", Rsdp->ExtendedChecksum);
    }
}
