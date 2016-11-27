#ifndef _MEM_H_
#define _MEM_H_

NTSTATUS
MmTranslateVirtualAddressWithCr3(
    _In_ QWORD Cr3,
    _In_ PVOID VirtualAddress,
    _Out_ QWORD *PhysicalAddress,
    _Out_opt_ DWORD *PageSize
);

#define MmTranslateSystemVirtualAddress(Va, Pa, Ps)     MmTranslateVirtualAddressWithCr3(__readcr3(), (Va), (Pa), (Ps))

#endif // !_MEM_H_
