#include "defs.h"
#include "memdefs.h"
#include "kernel.h"

KGLOBAL gKernelGlobalData;

VOID
KeInitGlobal(
    VOID
)
{
    memset(&gKernelGlobalData, 0, sizeof(gKernelGlobalData));
    
    gKernelGlobalData.PhysicalBase = KBASE_PHYSICAL;
    gKernelGlobalData.VirtualBase = KBASE_VIRTUAL;
    gKernelGlobalData.KernelSize = 32 * ONE_MB;
    gKernelGlobalData.Phase = 1;
}
