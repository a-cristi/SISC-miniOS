#include "defs.h"
#include "multiboot.h"
#include "mbflg.h"
#include "mb_util.h"
#include "log.h"

BOOLEAN
MbInterpretMultiBootInfo(
    _In_ PMULTIBOOT_INFO MultibootInfo
)
{
    BOOLEAN bValid = TRUE;

    if (0 != (MBOOT_FLG_LU_MEM & MultibootInfo->flags))
    {
        Log("Upper memory size: 0x%08x KB\n", MultibootInfo->mem_upper);
        Log("Lower memory size: 0x%08x KB\n", MultibootInfo->mem_lower);
    }
    else
    {
        Log("[FATAL ERROR] MBOOT_FLG_LU_MEM flag is not set. No memory information is available!\n");
        bValid = FALSE;
    }

    if (0 != (MBOOT_FLG_BOOT_DEV & MultibootInfo->flags))
    {
        Log("Boot device: 0x%08x => %d %d %d %d\n", MultibootInfo->boot_device,
            MB_BOOT_DEV_DRIVE(MultibootInfo->boot_device), MB_BOOT_DEV_PART1(MultibootInfo->boot_device),
            MB_BOOT_DEV_PART2(MultibootInfo->boot_device), MB_BOOT_DEV_PART3(MultibootInfo->boot_device));
    }
    else
    {
        Log("MBOOT_FLG_BOOT_DEV is not set.\n");
    }

    if (0 != (MBOOT_FLG_CMD_LINE & MultibootInfo->flags))
    {
        Log("Command line at PA %018p => %s\n", MultibootInfo->cmdline, MultibootInfo->cmdline);
    }
    else
    {
        Log("MBOOT_FLG_CMD_LINE is not set.\n");
    }

    if (0 != (MBOOT_FLG_MODS & MultibootInfo->flags))
    {
        Log("Modules count: %d\n", MultibootInfo->mods_count);
    }
    else
    {
        Log("MBOOT_FLG_MODS is not set.\n");
    }

    if ((0 != (MBOOT_FLG_AOUT_VALID & MultibootInfo->flags)) &&
        (0 != (MBOOT_FLG_ELF_VALID  & MultibootInfo->flags)))
    {
        Log("[FATAL] MBOOT_FLG_AOUT_VALID and MBOOT_FLG_ELF_VALID can not be both set!\n");
        bValid = FALSE;
    }

    if (0 != (MBOOT_FLG_MMAP & MultibootInfo->flags))
    {
        Log("Memory map present. Buffer starts at: %018p and has size: 0x%08x\n", 
            MultibootInfo->mmap_addr, MultibootInfo->mmap_length);
    }
    else
    {
        Log("[FATAL ERROR] MBOOT_FLG_MMAP is not set.\n");
        bValid = FALSE;
    }

    return bValid;
}

VOID
MbDumpMemoryMap(
    _In_ PMULTIBOOT_INFO MultibootInfo
)
{
    PMEMORY_MAP pMapEntry = (MEMORY_MAP *)(SIZE_T)MultibootInfo->mmap_addr;
    DWORD length = MultibootInfo->mmap_length;
    DWORD parsedLength = 0;

    while (parsedLength < length)
    {
        QWORD base = 0;
        QWORD end = 0;

        if (length - parsedLength < sizeof(MEMORY_MAP))
        {
            break;
        }

        base = ((QWORD)pMapEntry->base_addr_high << 32) | pMapEntry->base_addr_low;
        end = ((QWORD)pMapEntry->length_high << 32) | pMapEntry->length_low;
        end += base;

        Log("Region [%018p, %018p) with type: %d\n", 
            base, end, pMapEntry->type);
    
        parsedLength += pMapEntry->size + sizeof(pMapEntry->size);
        pMapEntry = (MEMORY_MAP *)((SIZE_T)MultibootInfo->mmap_addr + parsedLength);
    }
}