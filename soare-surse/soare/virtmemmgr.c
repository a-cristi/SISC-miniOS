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

#define VA2PML4(vaddr)           (VAS_VMGR_PML4)
#define VA2PDP(vaddr)            (VAS_VMGR_PDP + (((vaddr) >> 27) & 0x00001FF000))
#define VA2PD(vaddr)             (VAS_VMGR_PD + (((vaddr) >> 18) & 0x003FFFF000))
#define VA2PT(vaddr)             (VAS_VMGR_PT + (((vaddr) >> 9) & 0x7FFFFFF000))


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

    UNREFERENCED_PARAMETER(KernelPaStart);
    UNREFERENCED_PARAMETER(KernelVaStart);
    UNREFERENCED_PARAMETER(KernelRegionLength);

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

    return STATUS_SUCCESS;
}
