#include "defs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "mem.h"
#include "log.h"
#include "physmemmgr.h"

#define PTE_COUNT   512

typedef QWORD       PTE, *PPTE;

#pragma pack(push)
#pragma pack(1)

typedef struct _PT
{
    PTE     Entries[PTE_COUNT];
} PT, *PPT;

#pragma pack(pop)

#define PAGES_TO_PTES(pages)    (ROUND_UP((pages), PTE_COUNT) / PTE_COUNT)

static_assert(sizeof(PT) == PAGE_SIZE_4K, "PT size not 4K!");

#define VA_SPACE_VIRTMMGR       (ONE_TB * 2)



static QWORD gPhase1TableRangeStart;
static QWORD gPhase1TableRangeNextTable;
static QWORD gPhase1TableRangeEnd;
static QWORD gTableRangeDelta;


static
BOOLEAN
_MmPhase1CreateTableLayout(
    _In_ QWORD Pml4eCount,
    _In_ QWORD PdpeCount,
    _In_ QWORD PdeCount
)
{
#define MAX_TRY_COUNT   512
    QWORD pages = 1 + Pml4eCount + PdpeCount + PdeCount;
    QWORD startRange = 32 * ONE_MB;
    WORD tryCount = 0;
    
    while (tryCount < MAX_TRY_COUNT)
    {
        NTSTATUS status = MmReservePhysicalRange(startRange, pages * PAGE_SIZE_4K);
        if (NT_SUCCESS(status))
        {
            gPhase1TableRangeStart = startRange;
            gPhase1TableRangeNextTable = startRange;
            gPhase1TableRangeEnd = gPhase1TableRangeNextTable + pages * PAGE_SIZE_4K;
            return TRUE;
        }

        startRange += PAGE_SIZE_4K;
        tryCount++;
    }

    return FALSE;
}


static
BOOLEAN
_MmPhase1AllocTable(
    _Out_ PPT *Pt
)
{
    if (gPhase1TableRangeNextTable >= gPhase1TableRangeEnd)
    {
        return FALSE;
    }

    *Pt = (PT *)gPhase1TableRangeNextTable;
    memset(gPhase1TableRangeNextTable, 0, sizeof(PT));
    gPhase1TableRangeNextTable += sizeof(PT);

    return TRUE;
}


static
BOOLEAN
_MmPhase1GetTableForIndex(
    _In_ PPT CurrentTable,
    _In_ WORD Index,
    _Out_ PPT *NextTable
)
{
    PTE pte = CurrentTable->Entries[Index];
    PPT pPt = NULL;

    if (pte & PTE_P)
    {
        *NextTable = (PT *)CLEAN_PHYADDR(pte);
        return TRUE;
    }

    if (_MmPhase1AllocTable(&pPt))
    {
        CurrentTable->Entries[Index] = CLEAN_PHYADDR((QWORD)pPt) | PTE_P | PTE_RW | PTE_US;
        *NextTable = pPt;
        return TRUE;
    }

    return FALSE;
}


static
BOOLEAN
_MmPhase1MapPaToVa(
    _In_ PPT Pml4,
    _In_ QWORD Va,
    _In_ QWORD Pa
)
{
    WORD pml4i = 0;
    WORD pdpi = 0;
    WORD pdi = 0;
    WORD pti = 0;
    PPT pPdp = NULL;
    PPT pPd = NULL;
    PPT pPt = NULL;

    MmGetIndexesForVa((PVOID)Va, &pml4i, &pdpi, &pdi, &pti);

    if (!_MmPhase1GetTableForIndex(Pml4, pml4i, &pPdp))
    {
        return FALSE;
    }

    if (!_MmPhase1GetTableForIndex(pPdp, pdpi, &pPd))
    {
        return FALSE;
    }

    if (!_MmPhase1GetTableForIndex(pPd, pdi, &pPt))
    {
        return FALSE;
    }

    if (pPt->Entries[pti] & PTE_P)
    {
        return FALSE;
    }

    pPt->Entries[pti] = CLEAN_PHYADDR(Pa) | PTE_P | PTE_RW | PTE_US;

    return TRUE;
}


static
BOOLEAN
_MmPhase1MapPaRangeToVaRange(
    _In_ PPT Pml4,
    _In_ QWORD VaRangeStart,
    _In_ QWORD PaRangeStart,
    _In_ QWORD Length
)
{
    for (QWORD page = 0; page < Length; page += PAGE_SIZE_4K)
    {
        if (!_MmPhase1MapPaToVa(Pml4, VaRangeStart + page, PaRangeStart + page))
        {
            return FALSE;
        }
    }

    return TRUE;
}


NTSTATUS
MmVirtualManagerInit(
    _In_ QWORD MaximumMemorySize,
    _In_ QWORD KernelPaStart,
    _In_ QWORD KernelVaStart,
    _In_ QWORD KernelRegionLength
)
{
    QWORD pteCount;
    QWORD pdeCount;
    QWORD pdpeCount;
    QWORD pml4eCount;
    QWORD pagesNeeded;
    PPT pPml4;

    pteCount = SMALL_PAGE_COUNT(MaximumMemorySize); // how many pages we have
    pdeCount = PAGES_TO_PTES(pteCount);             // how many PTs we need
    pdpeCount = PAGES_TO_PTES(pdeCount);            // how many PDs we need
    pml4eCount = PAGES_TO_PTES(pdpeCount);          // how many PDPs we need

    if (pml4eCount > PTE_COUNT)
    {
        LogWithInfo("Too many PML4Es needed for 0x%llx bytes (%d MB): %d\n", 
            pml4eCount, MaximumMemorySize, ByteToMb(MaximumMemorySize));
        return STATUS_NOT_SUPPORTED;
    }

    pagesNeeded = 1 + pml4eCount + pdpeCount + pdeCount;

    Log("[VIRTMEM] In order to map 0x%llx bytes (%d MB) we need:\n", MaximumMemorySize, ByteToMb(MaximumMemorySize));
    Log("\t%d PML4 Entries, %d PDP Entries, %d PD Entries, %d PT Entries\n",
        pml4eCount, pdpeCount, pdeCount, pteCount);
    Log("\t%d pages (%d MB)\n", pagesNeeded, ByteToMb(pagesNeeded * PAGE_SIZE_4K));

    if (!_MmPhase1CreateTableLayout(pml4eCount, pdpeCount, pdeCount))
    {
        LogWithInfo("[ERROR] _MmPhase1CreateTableLayout failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!_MmPhase1AllocTable(&pPml4))
    {
        LogWithInfo("[ERROR] _MmPhase1AllocTable failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Log("[VIRTMEM] Final PDBR @ %018p\n", pPml4);

    if (!_MmPhase1MapPaRangeToVaRange(pPml4, VA_SPACE_VIRTMMGR, gPhase1TableRangeStart, gPhase1TableRangeEnd - gPhase1TableRangeStart))
    {
        LogWithInfo("[ERROR] _MmPhase1MapPaRangeToVaRange failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Log("[VIRTMEM] Mapped [%018p %018p) VA -> [%018p, %018p) PA\n",
        VA_SPACE_VIRTMMGR, VA_SPACE_VIRTMMGR + gPhase1TableRangeEnd - gPhase1TableRangeStart,
        gPhase1TableRangeEnd, gPhase1TableRangeEnd + gPhase1TableRangeEnd - gPhase1TableRangeStart);

    if (!_MmPhase1MapPaRangeToVaRange(pPml4, KernelVaStart, KernelPaStart, KernelRegionLength))
    {
        LogWithInfo("[ERROR] _MmPhase1MapPaRangeToVaRange failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Log("[VIRTMEM] Mapped [%018p %018p) VA -> [%018p, %018p) PA\n",
        KernelVaStart, KernelVaStart + KernelRegionLength,
        KernelPaStart, KernelPaStart + KernelRegionLength);

    return STATUS_SUCCESS;
}
