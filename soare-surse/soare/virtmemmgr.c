#include "defs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "log.h"
#include "physmemmgr.h"

#define PTE_COUNT   512

typedef QWORD       PTE, *PPTE;

#pragma pack(push)
#pragma pack(1)

typedef struct _PT
{
    PTE             Entries[PTE_COUNT];
} PT, *PPT;

#pragma pack(pop)

#define PAGES_TO_PTES(pages)    (ROUND_UP((pages), PTE_COUNT) / PTE_COUNT)

static_assert(sizeof(PT) == PAGE_SIZE_4K, "PT size not 4K!");


static
NTSTATUS
_MmGetFreePtEntryInOneToOneMapping(
    _Out_ PPT *Pt,
    _Out_ DWORD *Index
)
//
// Does a linear search inside the temporary one-to-one page tables
// IMPORTANT: do not use after we switch to the final page tables!
//
{
    PPT pPml4;

    pPml4 = (PT *)(__readcr3() & PHYS_PAGE_MASK);

    for (DWORD pml4i = 0; pml4i < PTE_COUNT; pml4i++)
    {
        PPT pPdp;
        PTE pml4e = pPml4->Entries[pml4i];

        // skip non present entries
        if (!(pml4e & PML4E_P))
        {
            continue;
        }

        pPdp = (PT *)(pml4e & PHYS_PAGE_MASK);

        for (DWORD pdpi = 0; pdpi < PTE_COUNT; pdpi++)
        {
            PPT pPd;
            PTE pdpe = pPdp->Entries[pdpi];

            // skip non present and huge page entries
            if ((!(pdpe & PDPE_P)) || (pdpe & PDPE_PS))
            {
                continue;
            }

            pPd = (PT *)(pdpe & PHYS_PAGE_MASK);

            for (DWORD pdi = 0; pdi < PTE_COUNT; pdi++)
            {
                PPT pPt;
                PTE pde = pPd->Entries[pdi];

                // skip non present and huge page entries
                if ((!(pde & PDE_P)) || (pde & PDE_PS))
                {
                    continue;
                }

                pPt = (PT *)(pde & PHYS_PAGE_MASK);

                for (DWORD pti = 0; pti < PTE_COUNT; pti++)
                {
                    PTE pte = pPt->Entries[pti];

                    // we want a free entry
                    if (!(pte & PTE_P))
                    {
                        *Pt = pPt;
                        *Index = pti;
                        return STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    return STATUS_NOT_FOUND;
}


NTSTATUS
MmVirtualManagerInit(
    _In_ QWORD MaximumMemorySize
)
{
    QWORD pteCount;
    QWORD pdeCount;
    QWORD pdpeCount;
    QWORD pml4eCount;

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

    Log("[VIRTMEM] In order to map 0x%llx bytes (%d MB) we need:\n", MaximumMemorySize, ByteToMb(MaximumMemorySize));
    Log("\t%d PML4 Entries, %d PDP Entries, %d PD Entries, %d PT Entries\n",
        pml4eCount, pdpeCount, pdeCount, pteCount);

    return STATUS_SUCCESS;
}
