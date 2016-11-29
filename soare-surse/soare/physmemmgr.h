#ifndef _PHYSMEMMGR_H_
#define _PHYSMEMMGR_H_

BOOLEAN
MmPhysicalManagerInit(
    _In_ PVOID BitmapAddress
);

NTSTATUS
MmReservePhysicalPage(
    _In_ QWORD Page
);

NTSTATUS
MmReservePhysicalRange(
    _In_ QWORD Base,
    _In_ QWORD Length
);

NTSTATUS
MmFreePhysicalPage(
    _In_ QWORD Page
);

NTSTATUS
MmAllocPhysicalPage(
    _Inout_ QWORD * Page
);

QWORD
MmGetTotalFreeMemory(
    VOID
);

BOOLEAN
MmIsPhysicalPageFree(
    _In_ QWORD Page
);

QWORD
MmGetTotalNumberOfPhysicalPages(
    VOID
);

#endif // !_PHYSMEMMGR_H_
