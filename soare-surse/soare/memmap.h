#ifndef _MEM_MAP_H_
#define _MEM_MAP_H_

typedef enum _MEM_TYPE
{
    memTypeUnknown = 0,
    memTypeUsable,
    memTypeReserved,
    memTypeAcpiReclaimable,
    memTypeAcpiNvs,
    memTypeBad,
    memTypeLast = memTypeBad
} MEM_TYPE, *PMEM_TYPE;

typedef struct _MMAP_ENTRY
{
    QWORD       Base;
    QWORD       Length;
    MEM_TYPE    Type;
} MMAP_ENTRY, *PMMAP_ENTRY;

#define MAX_MMAP_ENTRIES    128

VOID
MmDumpMemoryMap(
    _In_ PMMAP_ENTRY Map,
    _In_ DWORD Count
);

VOID
MmInitMemoryMapFromMultiboot(
    _In_ QWORD MapAddress,
    _In_ DWORD MapLength
);

NTSTATUS
MmGetMapEntryForAddress(
    _In_ QWORD PhysicalAddress,
    _Out_ MMAP_ENTRY * Entry
);

#endif // !_MEM_MAP_H_
