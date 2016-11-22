#include "defs.h"
#include "memmap.h"
#include "multiboot.h"
#include "memdefs.h"
#include "log.h"

MMAP_ENTRY gBootMemoryMap[MAX_MMAP_ENTRIES];
DWORD gBootMemoryMapEntries;
SIZE_T gBootMemoryLimit;

PCHAR
MmMemoryTypeToString(
    _In_ MEM_TYPE Type
)
{
    switch (Type)
    {
    case memTypeUsable: return "Usable";
    case memTypeReserved: return "Reserved";
    case memTypeAcpiReclaimable: return "ACPI Reclaimable";
    case memTypeAcpiNvs: return "ACPI NVS";
    case memTypeBad: return "Bad";
    default: return "Unknown";
    }
}


VOID
MmDumpMemoryMap(
    _In_ PMMAP_ENTRY Map,
    _In_ DWORD Count
)
{
    for (DWORD i = 0; i < Count; i++)
    {
        PMMAP_ENTRY pEntry = &Map[i];

        Log("[%d / %d] [%018p, %018p) is %d (%s)\n", i, Count, pEntry->Base, 
            pEntry->Base + pEntry->Length, pEntry->Type, MmMemoryTypeToString(pEntry->Type));
    }
}


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
    gBootMemoryLimit = 0;

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
        gBootMemoryMap[gBootMemoryMapEntries].Type = pEntry->type > memTypeLast ? 0 : pEntry->type;

        parsedLength += pEntry->size + sizeof(pEntry->size);
        pEntry = (MEMORY_MAP *)((SIZE_T)MapAddress + parsedLength);

        gBootMemoryMapEntries++;
    }

    gBootMemoryLimit = gBootMemoryMap[gBootMemoryMapEntries - 1].Base + gBootMemoryMap[gBootMemoryMapEntries - 1].Length;
    
    Log("[MMAP] Parsed %d entries for a total size of 0x%llx Bytes (%d MB)\n", 
        gBootMemoryMapEntries, gBootMemoryLimit, ByteToMb(gBootMemoryLimit));
    MmDumpMemoryMap(gBootMemoryMap, gBootMemoryMapEntries);
}