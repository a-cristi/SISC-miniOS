#include "defs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "log.h"

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
