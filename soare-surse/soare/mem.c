#include "defs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "mem.h"
#include "log.h"
#include "kernel.h"

extern KGLOBAL gKernelGlobalData;

NTSTATUS
MmTranslateVirtualAddressWithCr3(
    _In_ QWORD Cr3,
    _In_ PVOID VirtualAddress,
    _Out_ QWORD *PhysicalAddress,
    _Out_opt_ DWORD *PageSize
)
{
    DWORD pageSize = 0;
    QWORD pa = 0;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Cr3);

    if (!PhysicalAddress)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    if (gKernelGlobalData.Phase <= 1)
    {
        if ((QWORD)VirtualAddress >= gKernelGlobalData.VirtualBase)
        {
            pa = (QWORD)VirtualAddress - (gKernelGlobalData.VirtualBase - gKernelGlobalData.PhysicalBase);
        }
        else
        {
            pa = (QWORD)VirtualAddress;
        }

        pageSize = PAGE_SIZE_4K;
        status = STATUS_SUCCESS;
        goto _cleanup_and_exit;
    }
    else
    {
        status = STATUS_NOT_IMPLEMENTED;
        goto _cleanup_and_exit;
    }

_cleanup_and_exit:
    if (NT_SUCCESS(status))
    {
        if (PageSize)
        {
            *PageSize = pageSize;
        }

        *PhysicalAddress = pa;
    }

    return status;
}


VOID
MmGetIndexesForVa(
    _In_ PVOID VirtualAddress,
    _Out_opt_ WORD *Pml4Index,
    _Out_opt_ WORD *PdpIndex,
    _Out_opt_ WORD *PdIndex,
    _Out_opt_ WORD *PtIndex
)
{
    QWORD va = (QWORD)VirtualAddress;

    if (Pml4Index)
    {
        *Pml4Index = (WORD)((va & PML4_IDX_MASK) >> PML4_IDX_SHIFT);
    }

    if (PdpIndex)
    {
        *PdpIndex = (WORD)((va & PDP_IDX_MASK) >> PDP_IDX_SHIFT);
    }

    if (PdIndex)
    {
        *PdIndex = (WORD)((va & PD_IDX_MASK) >> PD_IDX_SHIFT);
    }

    if (PtIndex)
    {
        *PtIndex = (WORD)((va & PT_IDX_MASK) >> PT_IDX_SHIFT);
    }
}
