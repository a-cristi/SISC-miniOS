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
