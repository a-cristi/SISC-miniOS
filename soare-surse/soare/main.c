#include "defs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "boot.h"
#include "screen.h"
#include "log.h"
#include "pic.h"
#include "multiboot.h"
#include "mb_util.h"
#include "kernel.h"
#include "memmap.h"
#include "physmemmgr.h"
#include "virtmemmgr.h"
#include "panic.h"
#include "dtr.h"
#include "debugger.h"

extern KGLOBAL gKernelGlobalData;

void EntryPoint(
    _In_ PMULTIBOOT_INFO MultiBootInfo
)
{
    NTSTATUS status;
    QWORD totalMemory;
    QWORD pmmgrStart;
    QWORD pmmgEnd;
#define GUARD_VALUE 'grd0'
    DWORD guard = GUARD_VALUE;

    VgaInit(VGA_MEMORY_BUFFER, vgaColorWhite, vgaColorBlack);
    Log("Built on %s %s\n", __DATE__, __TIME__);

    // make sure we are at 1T
    if ((SIZE_T)&EntryPoint < KBASE_VIRTUAL)
    {
        Log("[FATAL ERROR] We are not using the expected VA space!\n");
        PANIC(" We are not using the expected VA space");
    }

    Log("Multiboot info @ %018p\n", MultiBootInfo);
    if (!MbInterpretMultiBootInfo(MultiBootInfo))
    {
        VgaSetForeground(vgaColorRed);
        Log("[FATAL ERROR] Not enough information is available to boot the OS!\n");
        PANIC("Incomplete boot information\n");
    }

    KeInitGlobal();

    MbDumpMemoryMap(MultiBootInfo);
    MmInitMemoryMapFromMultiboot(MultiBootInfo->mmap_addr, MultiBootInfo->mmap_length);

    totalMemory = (MultiBootInfo->mem_lower + MultiBootInfo->mem_upper) * ONE_KB;

    if (!MmPhysicalManagerInit((PVOID)(gKernelGlobalData.VirtualBase + gKernelGlobalData.KernelSize)))
    {
        Log("[FATAL ERROR] Failed to init the physical memory manager.\n");
        PANIC("Unable to initialize the physical memory manager\n");
    }

    // reserve the kernel space
    status = MmReservePhysicalRange(gKernelGlobalData.PhysicalBase, gKernelGlobalData.KernelSize);
    if (!NT_SUCCESS(status))
    {
        Log("[FATAL ERROR] Failed to reserve the kernel memory area.\n");
        PANIC("Unable to reserve enough physical memory for the kernel\n");
    }

    pmmgrStart = 0;
    pmmgEnd = 0;
    MmGetPmmgrReservedPhysicalRange(&pmmgrStart, &pmmgEnd);

    Log("%018p bytes (%d MB) of physical memory are available out of %018p bytes (%d MB)\n", 
        MmGetTotalFreeMemory(), ByteToMb(MmGetTotalFreeMemory()), totalMemory, ByteToMb(totalMemory));

    Log("Guard @ %018p = 0x%08x\n", &guard, guard);

    status = MmVirtualManagerInit(totalMemory, 
        gKernelGlobalData.PhysicalBase, gKernelGlobalData.VirtualBase,
        gKernelGlobalData.KernelSize + pmmgEnd - pmmgrStart);
    if (!NT_SUCCESS(status))
    {
        Log("[FATAL ERROR] MmVirtualManagerInit: 0x%08x\n", status);
        PANIC("Failed to initialize the virtual memory manager");
    }

    Log("Guard @ %018p = 0x%08x\n", &guard, guard);
    if (guard != GUARD_VALUE)
    {
        LogWithInfo("[FATAL ERROR] Stack guard value was corrupted!\n");
        PANIC("Stack guard value was corrupted!");
    }

    gKernelGlobalData.Phase = 2;    // memory manager initialized, BSP stack switched

    Log("> Setting exception handlers...");
    status = ExInitExceptionHandling(NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[FATAL ERROR] ExInitExceptionHandling failed: 0x%08x\n", status);
        PANIC("Unable to initialize the exception mechanism!");
    }
    Log("Done!\n");
    
    DbgBreak();

    Log("> Initializing PIC... ");
    PicInitialize();
    Log("Done!\n");

    DbgBreak();
    __halt();
}
