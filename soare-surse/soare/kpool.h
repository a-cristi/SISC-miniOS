#ifndef _KPOOL_H_
#define _KPOOL_H_

NTSTATUS
KpInit(
    _In_ PVOID Base,
    _In_ DWORD Length,
    _In_ DWORD EntrySize
);

NTSTATUS
KpAlloc(
    _Out_ PVOID *Ptr
);

NTSTATUS
KpFreeAndNull(
    _Inout_ PVOID *Ptr
);

#endif // !_KPOOL_H_
