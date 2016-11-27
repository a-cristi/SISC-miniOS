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

#endif // !_PHYSMEMMGR_H_
