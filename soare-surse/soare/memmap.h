#ifndef _MEM_MAP_H_
#define _MEM_MAP_H_

typedef enum _MEM_TYPE
{
    memTypeUnknown = 0,
    memTypeUsable,
    memTypeReserved,
    memTypeAcpiReclaimable,
    memTypeAcpiNvs,
    memTypeBad
} MEM_TYPE, *PMEM_TYPE;

typedef struct _MMAP_ENTRY
{
    QWORD       Base;
    QWORD       Length;
    MEM_TYPE    Type;
} MMAP_ENTRY, *PMMAP_ENTRY;

#endif // !_MEM_MAP_H_
