#include "defs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "mem.h"
#include "log.h"
#include "physmemmgr.h"
#include "debugger.h"

#define PTE_COUNT               512
#define PTE_RECURSIVE_INDEX     511ULL
#define PML4_INDEX(x)           (((x) >> PML4_IDX_SHIFT) & 0x1FF)
#define PDP_INDEX(x)            (((x) >> PDP_IDX_SHIFT) & 0x1FF)
#define PD_INDEX(x)             (((x) >> PD_IDX_SHIFT) & 0x1FF)
#define PT_INDEX(x)             (((x) >> PT_IDX_SHIFT) & 0x1FF)

#define VAS_VMGR_PT             (0xFFFF000000000000ULL  + (PTE_RECURSIVE_INDEX << 39))
#define VAS_VMGR_PD             (VAS_VMGR_PT            + (PTE_RECURSIVE_INDEX << 30))
#define VAS_VMGR_PDP            (VAS_VMGR_PD            + (PTE_RECURSIVE_INDEX << 21))
#define VAS_VMGR_PML4           (VAS_VMGR_PDP           + (PTE_RECURSIVE_INDEX << 12))

#define VA2PML4(vaddr)          (VAS_VMGR_PML4)
#define VA2PDP(vaddr)           (VAS_VMGR_PDP + (((vaddr) >> 27) & 0x00001FF000))
#define VA2PD(vaddr)            (VAS_VMGR_PD + (((vaddr) >> 18) & 0x003FFFF000))
#define VA2PT(vaddr)            (VAS_VMGR_PT + (((vaddr) >> 9) & 0x7FFFFFF000))

#define VAS_KERNEL              (ONE_TB)
#define VAS_LOWMEM              (0ULL)

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


static 
NTSTATUS
_MmPhase1GetNextTable(
    _In_ PPT CurrentTable,
    _In_ WORD Index,
    _Out_ PPT *NextTable
)
{
    PTE pte = CurrentTable->Entries[Index];

    // not present? create one entry
    if (!(pte & PTE_P))
    {
        QWORD pa = 0;
        NTSTATUS status = MmAllocPhysicalPage(&pa);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        memset((VOID *)pa, 0, PAGE_SIZE_4K);
        CurrentTable->Entries[Index] = CLEAN_PHYADDR(pa) | PTE_P | PTE_RW | PTE_US;
    }

    // this works only while we are in Phase1 when we use the one-to-one paging tables
    *NextTable = (PT *)(CLEAN_PHYADDR(CurrentTable->Entries[Index]));

    return STATUS_SUCCESS;
}


static
NTSTATUS
_MmPhase1MapContigousRegion(
    _In_ QWORD FinalPdbr,
    _In_ QWORD PhysicalBase,
    _In_ QWORD VirtualBase,
    _In_ QWORD Size
)
{
    PPT pPml4 = (PT *)FinalPdbr;
    QWORD pages = SMALL_PAGE_COUNT(Size);
    QWORD nextPa = PhysicalBase;
    QWORD nextVa = VirtualBase;

    for (QWORD i = 0; i < pages; i++)
    {
        NTSTATUS status;
        PPT pPdp = NULL;
        PPT pPd = NULL;
        PPT pPt = NULL;

        // PML4E -> PDP
        status = _MmPhase1GetNextTable(pPml4, PML4_INDEX(nextVa), &pPdp);
        if (!NT_SUCCESS(status))
        {
            LogWithInfo("[ERROR] _MmPhase1GetNextTable failed: 0x%08x\n", status);
            return status;
        }

        // PDPE -> PD
        status = _MmPhase1GetNextTable(pPdp, PDP_INDEX(nextVa), &pPd);
        if (!NT_SUCCESS(status))
        {
            LogWithInfo("[ERROR] _MmPhase1GetNextTable failed: 0x%08x\n", status);
            return status;
        }

        // PDE -> PT
        status = _MmPhase1GetNextTable(pPd, PD_INDEX(nextVa), &pPt);
        if (!NT_SUCCESS(status))
        {
            LogWithInfo("[ERROR] _MmPhase1GetNextTable failed: 0x%08x\n", status);
            return status;
        }

        if (0 != (pPt->Entries[PT_INDEX(nextVa)] & PTE_P))
        {
            LogWithInfo("[WARNING] PTE for VA %018p is already set to %018p!\n", nextVa, pPt->Entries[PT_INDEX(nextVa)]);
        }

        pPt->Entries[PT_INDEX(nextVa)] = CLEAN_PHYADDR(nextPa) | PTE_P | PTE_RW | PTE_US;

        // next virtual and physical page
        nextVa += PAGE_SIZE_4K;
        nextPa += PAGE_SIZE_4K;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MmMapContigousPhysicalRegion(
    _In_ QWORD PhysicalBase,
    _In_ QWORD VirtualBase,
    _In_ QWORD Size
)
{
    QWORD pages = SMALL_PAGE_COUNT(Size);
    QWORD nextVa = VirtualBase;
    QWORD nextPa = PhysicalBase;
    NTSTATUS status;
    PPT pPml4;
    PPT pPdp;
    PPT pPd;
    PPT pPt;

    if (PhysicalBase % PAGE_SIZE_4K)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (VirtualBase % PAGE_SIZE_4K)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    for (QWORD count = 0; count < pages; count++)
    {
        pPml4 = (PT *)VA2PML4(nextVa);

        if (!(pPml4->Entries[PML4_INDEX(nextVa)] & PML4E_P))
        {
            QWORD pa = 0;
            status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                LogWithInfo("[ERROR] MmAllocPhysicalPage failed: 0x%08x\n", status);
                return status;
            }

            memset(pPml4, 0, sizeof(PT));
            pPml4->Entries[PML4_INDEX(nextVa)] = CLEAN_PHYADDR(pa) | PTE_P | PTE_US | PTE_RW;
        }

        pPdp = (PT *)VA2PDP(nextVa);

        if (!(pPdp->Entries[PDP_INDEX(nextVa)] & PDPE_P))
        {
            QWORD pa = 0;
            status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                LogWithInfo("[ERROR] MmAllocPhysicalPage failed: 0x%08x\n", status);
                return status;
            }

            memset(pPdp, 0, sizeof(PT));
            pPdp->Entries[PDP_INDEX(nextVa)] = CLEAN_PHYADDR(pa) | PTE_P | PTE_US | PTE_RW;
        }

        pPd = (PT *)VA2PD(nextVa);

        if (!(pPd->Entries[PD_INDEX(nextVa)] & PDE_P))
        {
            QWORD pa = 0;
            status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                LogWithInfo("[ERROR] MmAllocPhysicalPage failed: 0x%08x\n", status);
                return status;
            }

            memset(pPd, 0, sizeof(PT));
            pPd->Entries[PD_INDEX(nextVa)] = CLEAN_PHYADDR(pa) | PTE_P | PTE_US | PTE_RW;
        }

        pPt = (PT *)VA2PT(nextVa);

        if (0 != (pPt->Entries[PT_INDEX(nextVa)] & PTE_P))
        {
            LogWithInfo("[WARNING] Overwriting PTE %018p for VA %018p!\n", pPt->Entries[PT_INDEX(nextVa)], nextVa);
        }

        pPt->Entries[PT_INDEX(nextVa)] = CLEAN_PHYADDR(nextPa) | PTE_P | PTE_US | PTE_RW;

        nextVa += PAGE_SIZE_4K;
        nextPa += PAGE_SIZE_4K;
    }

    return STATUS_SUCCESS;
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
    QWORD pdbr;
    NTSTATUS status;

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

    pdbr = 0;
    status = MmAllocPhysicalPage(&pdbr);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] MmAllocPhysicalPage failed: 0x%08x\n", status);
        return status;
    }

    pPml4 = (PT *)pdbr;
    Log("[VIRTMEM] PDBR @ %018p (PA)\n", pdbr);
    memset(pPml4->Entries, 0, sizeof(pPml4->Entries));

    // install the recursive entry
    pPml4->Entries[PTE_RECURSIVE_INDEX] = CLEAN_PHYADDR(pdbr) | PML4E_P | PML4E_RW | PML4E_US;
    Log("%018p\n", (QWORD)&MaximumMemorySize - (QWORD)&status);

    // also make the current PDBR recursive
    pPml4 = (PT *)CLEAN_PHYADDR(__readcr3());
    pPml4->Entries[PTE_RECURSIVE_INDEX] = CLEAN_PHYADDR(__readcr3()) | PML4E_P | PML4E_RW | PML4E_US;

    // map the kernel
    status = _MmPhase1MapContigousRegion(pdbr, KernelPaStart, KernelVaStart, KernelRegionLength);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] _MmPhase1MapContigousRegion failed: 0x%08x\n", status);
        return status;
    }

    Log("[VIRTMEM] Mapped the kernel: [%018p, %018p) -> [%018p, %018p)\n",
        KernelPaStart, KernelPaStart + KernelRegionLength, KernelVaStart, KernelVaStart + KernelRegionLength);

    // map low memory (4K - 2M)
    status = _MmPhase1MapContigousRegion(pdbr, ONE_KB * 4, ONE_KB * 4, 2 * ONE_MB - ONE_KB * 4);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] _MmPhase1MapContigousRegion failed: 0x%08x\n", status);
        return status;
    }

    // switch to the final PDBR
    Log("[VIRTMEM] Switching PDBR from %018p to %018p...\n", __readcr3(), pdbr);
    __writecr3(pdbr);
    Log("[VIRTMEM] PDBR switched to: %018p\n", __readcr3());

    return STATUS_SUCCESS;
}
