#ifndef _VIRTMEMMGR_H_
#define _VIRTMEMMGR_H_

NTSTATUS
MmVirtualManagerInit(
    _In_ QWORD MaximumMemorySize,
    _In_ QWORD KernelPaStart,
    _In_ QWORD KernelVaStart,
    _In_ QWORD KernelRegionLength
);

NTSTATUS
MmMapContigousPhysicalRegion(
    _In_ QWORD PhysicalBase,
    _In_ QWORD VirtualBase,
    _In_ QWORD Size
);

NTSTATUS
MmTranslateVa(
    _In_ PVOID Va,
    _Out_ QWORD *Pa,
    _Out_ DWORD *PageSize
);

#endif // !_VIRTMEMMGR_H_
