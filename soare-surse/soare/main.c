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
#include "panic.h"
#include "debugger.h"

extern KGLOBAL gKernelGlobalData;

void EntryPoint(
    _In_ PMULTIBOOT_INFO MultiBootInfo
)
{
    NTSTATUS status;

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

    Log("%018p bytes (%d MB) of physical memory are available\n", MmGetTotalFreeMemory(), ByteToMb(MmGetTotalFreeMemory()));

    Log("> Initializing PIC... ");
    PicInitialize();
    Log("Done!\n");

    DbgBreak();
    __halt();
}
