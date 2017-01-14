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

#define MAP_FLG_SKIP_PHYPAGE_CHECK      0x0001  // don't reserve and don't check if the physical page is already checked

NTSTATUS
MmMapPhysicalPages(
    _In_ QWORD PhysicalBase,
    _In_ DWORD RangeSize,
    _Out_ PVOID *Ptr,
    _In_ DWORD Flags                    // MAP_FLG_*
);

NTSTATUS
MmUnmapRangeAndNull(
    _Inout_ PVOID *Ptr,
    _In_ DWORD Length,
    _In_ DWORD Flags                    // MAP_FLG_* (must match the ones used at mapping)
);

VOID
MmDumpVas(
    _In_ QWORD VaBase,
    _In_ QWORD Length
);

#endif // !_VIRTMEMMGR_H_
