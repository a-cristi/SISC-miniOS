#include "boot.h"
#include "screen.h"
#include "log.h"
#include "pic.h"
#include "multiboot.h"
#include "mb_util.h"
#include "memmap.h"
#include "physmemmgr.h"

extern void __DbgBochsBreak(void);

void EntryPoint(
    _In_ PMULTIBOOT_INFO MultiBootInfo
)
{
    VgaInit(VGA_MEMORY_BUFFER, vgaColorWhite, vgaColorBlack);
    Log("Built on %s %s\n", __DATE__, __TIME__);

    Log("Multiboot info @ %018p\n", MultiBootInfo);
    if (!MbInterpretMultiBootInfo(MultiBootInfo))
    {
        VgaSetForeground(vgaColorRed);
        Log("[FATAL ERROR] Not enough information is available to boot the OS!\n");
        __DbgBochsBreak();
        __halt();
    }

    MbDumpMemoryMap(MultiBootInfo);
    MmInitMemoryMapFromMultiboot(MultiBootInfo->mmap_addr, MultiBootInfo->mmap_length);

    if (!MmPhysicalManagerInit())
    {
        Log("[FATAL ERROR] Failed to init the physical memory manager.\n");
        __DbgBochsBreak();
        __halt();
    }

    Log("> Initializing PIC... ");
    PicInitialize();
    Log("Done!\n");
    __DbgBochsBreak();
    __halt();
}
