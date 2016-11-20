#include "defs.h"
#include "memmap.h"
#include "multiboot.h"
#include "log.h"

#define MAX_MMAP_ENTRIES    128

static MMAP_ENTRY gBootMemoryMap[MAX_MMAP_ENTRIES];
static DWORD gBootMemoryMapEntries;

VOID
MmInitMemoryMapFromMultiboot(
    _In_ QWORD MapAddress,
    _In_ DWORD MapLength
)
{
    DWORD parsedLength = 0;
    PMEMORY_MAP pEntry = (MEMORY_MAP *)(SIZE_T)MapAddress;

    memset(gBootMemoryMap, 0, sizeof(gBootMemoryMap));
    gBootMemoryMapEntries = 0;

    while (parsedLength < MapLength)
    {
        if (MapLength - parsedLength < sizeof(MEMORY_MAP))
        {
            break;
        }

        if (gBootMemoryMapEntries >= MAX_MMAP_ENTRIES)
        {
            Log("[WARNING] Too many memory map entries!\n");
            break;
        }

        gBootMemoryMap[gBootMemoryMapEntries].Base = ((QWORD)pEntry->base_addr_high << 32) | pEntry->base_addr_low;
        gBootMemoryMap[gBootMemoryMapEntries].Length = ((QWORD)pEntry->length_high << 32) | pEntry->length_low;
        gBootMemoryMap[gBootMemoryMapEntries].Type = pEntry->type;

        parsedLength += pEntry->size + sizeof(pEntry->size);
        pEntry = (MEMORY_MAP *)((SIZE_T)MapAddress + parsedLength);

        gBootMemoryMapEntries++;
    }

    Log("[MMAP] Parsed %d entries\n", gBootMemoryMapEntries);
}