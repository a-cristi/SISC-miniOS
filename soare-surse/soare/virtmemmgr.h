#ifndef _VIRTMEMMGR_H_
#define _VIRTMEMMGR_H_

NTSTATUS
MmVirtualManagerInit(
    _In_ QWORD MaximumMemorySize,
    _In_ QWORD KernelPaStart,
    _In_ QWORD KernelVaStart,
    _In_ QWORD KernelRegionLength
);

#endif // !_VIRTMEMMGR_H_
