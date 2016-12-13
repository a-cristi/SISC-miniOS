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
#include "timer.h"
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
    PPCPU pBsp = NULL;

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
    status = DtrCreatePcpu(&pBsp);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[FATAL ERROR] DtrCreatePcpu failed: 0x%08x\n", status);
        PANIC("Failed to create the BSP structure!");
    }
    Log("BSP CPU page @ %018p\n", pBsp);
    pBsp->IsBsp = TRUE;
    pBsp->Number = 0;
    pBsp->Self = pBsp;
    status = DtrInitAndLoadAll(pBsp);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] DtrInitAndLoadAll failed: 0x%08x\n", status);
        PANIC("Failed to initialize the BSP!");
    }

    Log("> Initializing PIC...\n");
    PicInitialize();
    Log("\t PIC Initialized!\n");

    Log("> Initializing the timer...\n");
    status = TmrInitializeTimer();
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("\n[ERROR] TmrInitializeTimer failed: 0x%08x\n", status);
        PANIC("Failed to initilize the system timer!");
    }
    Log("\t Timer Initialized!\n");
    {
        extern BYTE gImrMaster;
        extern BYTE gImrSlave;

        Log("IMRs 0x%02x 0x%02x\n", gImrMaster, gImrSlave);
    }
    DbgBreak();
    {
        volatile SIZE_T prev = 0;
        DWORD c = 0;
        BYTE value = 0;
        BYTE s = 0;
        BYTE m = 0;
        BYTE h = 0;
        BYTE rb = 0;

        do
        {
            __outbyte(0x70, 10 | 0x80);
            value = __inbyte(0x71);
        } while (0 != (value & 0x80));
        do
        {
            __outbyte(0x70, 10 | 0x80);
            value = __inbyte(0x71);
        } while (0 == (value & 0x80));
        do
        {
            __outbyte(0x70, 10 | 0x80);
            value = __inbyte(0x71);
        } while (0 != (value & 0x80));
        __outbyte(0x70, 0 | 0x80);
        s = __inbyte(0x71);
        __outbyte(0x70, 2 | 0x80);
        m = __inbyte(0x71);
        __outbyte(0x70, 4 | 0x80);
        h = __inbyte(0x71);
        __outbyte(0x70, 11 | 0x80);
        rb = __inbyte(0x71);

        if (0 == (rb & 0x04))
        {
            s = (s & 0x0F) + ((s / 16) * 10);
            m = (m & 0x0F) + ((m / 16) * 10);
            h = ((h & 0x0F) + (((h & 0x70) / 16) * 10)) | (h & 0x80);
            //d = (d & 0x0F) + ((d / 16) * 10);
            //m = (m & 0x0F) + ((m / 16) * 10);
            //y = (y & 0x0F) + ((y / 16) * 10)
        }

        if ((0 == (rb & 0x02)) && (0 != (h & 0x80)))
        {
            h = ((h & 0x7F) + 12) % 24;
        }

        Log("%02d::%02d::%02d\n", h, m, s);
        _enable();
        while (TRUE)
        {
            extern volatile SIZE_T gPitTickCount;
            if (gPitTickCount - prev >= 5965)
            {
                if (s < 59)
                {
                    s++;
                }
                else
                {
                    s = 0;
                    if (m < 59)
                    {
                        m++;
                    }
                    else
                    {
                        m = 0;
                        if (h < 23)
                        {
                            h++;
                        }
                        else
                        {
                            h = 0;
                        }
                    }
                }
                prev = gPitTickCount;
                //Log("%d\n", c);
                Log("%d %02d::%02d::%02d\n", c, h, m, s);
                c++;
            }
            //Log(".");
        }
    }
    DbgBreak();
    __halt();
}
