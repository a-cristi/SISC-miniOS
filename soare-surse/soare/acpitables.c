#include "defs.h"
#include "ntstatus.h"
#include "varargs.h"
#include "log.h"
#include "string.h"
#include "acpitables.h"
#include "virtmemmgr.h"
#include "dtr.h"
#include "msrdefs.h"
#include "panic.h"

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


NTSTATUS
AcpiParseXRsdt(
    _In_ QWORD TablePhysicalAddress,
    _In_ BOOLEAN Extended
)
{
    NTSTATUS status;
    PVOID pSdt = NULL;
    QWORD entries;
    DWORD tableSize = Extended ? sizeof(XSDT_TABLE) : sizeof(RSDT_TABLE);
    PRSDT_TABLE pRsdt = NULL;
    PXSDT_TABLE pXsdt = NULL;
    PACPI_TABLE_HEADER pCommonHeader = NULL;

    status = MmMapPhysicalPages(TablePhysicalAddress, tableSize, &pSdt, MAP_FLG_SKIP_PHYPAGE_CHECK);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] MmMapPhysicalPages failed for %p: 0x%x\n", TablePhysicalAddress, status);
        goto _cleanup_and_exit;
    }

    Log("[ACPI] Mapped %s (%018p) @ %018p with size 0x%08x\n", 
        Extended ? "XSDT" : "RSDT", TablePhysicalAddress, pSdt, tableSize);

    if (Extended)
    {
        pXsdt = (XSDT_TABLE *)pSdt;
        entries = (pXsdt->Header.Length - sizeof(pXsdt->Header)) / sizeof(pXsdt->TableOffsetEntry[0]);
        pCommonHeader = &pXsdt->Header;
    }
    else
    {
        pRsdt = (RSDT_TABLE *)pSdt;
        entries = (pRsdt->Header.Length - sizeof(pRsdt->Header)) / sizeof(pRsdt->TableOffsetEntry[0]);
        pCommonHeader = &pRsdt->Header;
    }

    if (AcpiGetTableChecksum((BYTE *)pCommonHeader, pCommonHeader->Length))
    {
        LogWithInfo("[ERROR] AcpiGetTableChecksum failed for %s\n", Extended ? "XSDT" : "RSDT");
        status = STATUS_INVALID_CHECKSUM;
        goto _cleanup_and_exit;
    }

    Log("[ACPI] Succesfully validated the %s Checksum!\n", Extended ? "XSDT" : "RSDT");

    Log("[ACPI] Dumping %s Header...\n", Extended ? "XSDT" : "RSDT");
    AcpiDumpTableHeader(pCommonHeader);

    Log("[ACPI] %s Entries count: %d\n", Extended ? "XSDT" : "RSDT", entries);

    for (QWORD i = 0; i < entries; i++)
    {
        QWORD headerPa = Extended ? pXsdt->TableOffsetEntry[i] : pRsdt->TableOffsetEntry[i];
        PACPI_TABLE_HEADER pHeader = NULL;

        Log("[ACPI] %s[%d] = %018p\n", Extended ? "XSDT" : "RSDT", i, headerPa);
        status = MmMapPhysicalPages(headerPa, sizeof(ACPI_TABLE_HEADER), &pHeader, MAP_FLG_SKIP_PHYPAGE_CHECK);
        if (!NT_SUCCESS(status))
        {
            LogWithInfo("[ERROR] MmMapPhysicalPages failed for %018p: 0x%08x\n", headerPa, status);
            goto _cleanup_and_exit;
        }

        if (*(UINT32 *)ACPI_SIG_MADT == *(UINT32 *)pHeader->Signature)
        {
            Log("[ACPI] Found MADT @ index %d inside the %s: %018p\n", i, Extended ? "XSDT" : "RSDT", headerPa);
            status = AcpiParseMadt(headerPa, pHeader->Length);
            if (!NT_SUCCESS(status))
            {
                LogWithInfo("[ERROR] _HvAcpiParseApicMadt failed: 0x%x\n", status);
            }
        }
        else
        {
            Log("[ACPI] Skipping %c%c%c%c...\n",
                pHeader->Signature[0], pHeader->Signature[1], pHeader->Signature[2], pHeader->Signature[3]);
        }

        MmUnmapRangeAndNull(&pHeader, sizeof(ACPI_TABLE_HEADER), MAP_FLG_SKIP_PHYPAGE_CHECK);
        if (!NT_SUCCESS(status))
        {
            // propagate the error returned by AcpiParseApicMadt
            goto _cleanup_and_exit;
        }
    }

_cleanup_and_exit:
    if (NULL != pSdt)
    {
        MmUnmapRangeAndNull(&pSdt, tableSize, MAP_FLG_SKIP_PHYPAGE_CHECK);
    }

    return status;
}


VOID
AcpiDumpTableHeader(
    _In_ PACPI_TABLE_HEADER Header
)
{
    if (Header)
    {
        Log("\t * Signature:    ");
        for (BYTE i = 0; i < ACPI_NAME_SIZE; i++)
        {
            Log("%c", Header->Signature[i]);
        }
        Log("\n");
        Log("\t * Length:       0x%x\n", Header->Length);
        Log("\t * Revision:     %d - %s\n", Header->Revision,
            0 == Header->Revision ? "ACPI 1.0" : 2 <= Header->Revision ? "ACPI 2.0+" : "Unknown");
        Log("\t * Checksum:     0x%x\n", Header->Checksum);
        Log("\t * OEM Id:       ");
        for (BYTE i = 0; i < ACPI_OEM_ID_SIZE; i++)
        {
            Log("%c", Header->OemId[i]);
        }
        Log("\n");
        Log("\t * OEM Table Id: ");
        for (BYTE i = 0; i < ACPI_OEM_TABLE_ID_SIZE; i++)
        {
            Log("%c", Header->OemTableId[i]);
        }
        Log("\n");
        Log("\t * OEM Revision: %d\n", Header->OemRevision);
        Log("\t * ASL Compiler: ");
        for (BYTE i = 0; i < ACPI_NAME_SIZE; i++)
        {
            Log("%c", Header->AslCompilerId[i]);
        }
        Log(" - Revision: %d\n", Header->AslCompilerRevision);
    }
}


NTSTATUS
AcpiParseMadt(
    _In_ QWORD MadtPhysicalAddress,
    _In_ SIZE_T Size
)
{
    PACPI_TABLE_HEADER pHeader = NULL;
    PMADT_LOCAL_APIC_TABLE pMadt;
    QWORD apicAddress;
    NTSTATUS status;

    status = MmMapPhysicalPages(MadtPhysicalAddress, 
        sizeof(ACPI_TABLE_HEADER) + sizeof(MADT_LOCAL_APIC_TABLE), 
        &pHeader, MAP_FLG_SKIP_PHYPAGE_CHECK);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] MmMapPhysicalPages failed for %018p: 0x%08x\n", MadtPhysicalAddress, status);
        goto _cleanup_and_exit;
    }

    pMadt = (PMADT_LOCAL_APIC_TABLE)((SIZE_T)pHeader + sizeof(ACPI_TABLE_HEADER) + 2 * sizeof(DWORD));
    apicAddress = *(DWORD *)((SIZE_T)pHeader + sizeof(ACPI_TABLE_HEADER));

    Log("[APIC] MADT @ %p\n", pMadt);
    Log("[ACPI] Obtained Local APIC address %p from table header.\n", apicAddress);

    while ((SIZE_T)pMadt < (SIZE_T)pHeader + Size)
    {
        switch (pMadt->Header.Type)
        {
        case MADT_TYPE_LOCAL_APIC:
        {
            PPCPU pBsp = GetCurrentCpu();

            Log("[ACPI] Found ACPI_MADT_TYPE_LOCAL_APIC @ %p\n", pMadt);
            Log("\t\t ACPI Processor ID: %d\n", pMadt->ProcessorId);
            Log("\t\t Local APIC ID:     %d\n", pMadt->Id);
            Log("\t\t LAPIC Flags:       0x%x\n", pMadt->LapicFlags);

            if (pBsp->ApicId != pMadt->Id)
            {
                PPCPU pPcpu = NULL;
                status = DtrAllocPcpu(&pPcpu);
                if (NT_SUCCESS(status))
                {
                    pPcpu->ApicId = pMadt->Id;
                    pPcpu->IsBsp = FALSE;
                    pPcpu->Number = (DWORD)-1;                     // not yet initialized
                    pPcpu->Self = pPcpu;
                    
                    // copy the IDT, GDT and TSS from the BSP
                    memcpy(&pPcpu->Gdt, &pBsp->Gdt, sizeof(pBsp->Gdt));
                    memcpy(&pPcpu->Gdtr, &pBsp->Gdtr, sizeof(pBsp->Gdtr));
                    memcpy(&pPcpu->Idt, &pBsp->Idt, sizeof(pBsp->Idt));
                    memcpy(&pPcpu->Idtr, &pBsp->Idtr, sizeof(pBsp->Idtr));
                    memcpy(&pPcpu->Tss, &pBsp->Tss, sizeof(pBsp->Tss));
                    memcpy(&pPcpu->Tr, &pBsp->Tr, sizeof(pBsp->Tr));
                }
                else
                {
                    LogWithInfo("[ERROR] DtrAllocPcpu failed: 0x%08x\n", status);
                    PANIC("Failed to allocate a new PCPU!\n");
                }
            }
        }

            break;

        default:
            Log("[ACPI] Skipping type %d...\n", pMadt->Header.Type);
            break;
        }

        pMadt = (PMADT_LOCAL_APIC_TABLE)((SIZE_T)pMadt + pMadt->Header.Length);
    }

_cleanup_and_exit:
    if (NULL != pHeader)
    {
        MmUnmapRangeAndNull(&pHeader, sizeof(ACPI_TABLE_HEADER) + sizeof(MADT_LOCAL_APIC_TABLE), MAP_FLG_SKIP_PHYPAGE_CHECK);
        pMadt = NULL;
    }

    return status;
}
