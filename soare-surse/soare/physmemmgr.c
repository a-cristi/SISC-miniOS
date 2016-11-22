#include "defs.h"
#include "memdefs.h"
#include "memmap.h"
#include "physmemmgr.h"
#include "log.h"

extern MMAP_ENTRY gBootMemoryMap[MAX_MMAP_ENTRIES];
extern DWORD gBootMemoryMapEntries;
extern SIZE_T gBootMemoryLimit;

BOOLEAN
MmA20IsEnabled(
    VOID
)
{
    return 0x02 & __inbyte(0x92);
}


BOOLEAN
MmA20Enable(
    VOID
)
{
    BYTE b = __inbyte(0x92);
    if (0 != (0x02 & b))
    {
        return TRUE;
    }

    // set bit 1
    b |= 0x02;
    // clear bit 0 (so we don't reset the system)
    b &= 0xFE;
    
    __outbyte(0x92, b);

    return MmA20IsEnabled();
}

BOOLEAN
MmPhysicalManagerInit(
    _In_ PVOID BitmapAddress
)
{
    QWORD endOfMemory = 0;

    if (!MmA20Enable())
    {
        Log("[FATAL ERROR] Failed to enable the A20 line!\n");
        return FALSE;
    }

    Log("[PHYSMEM] A20 line is enabled.\n");

    // compute the last valid memory address
    endOfMemory = gBootMemoryLimit;

    Log("[PHYSMEM] Will have to manage %018p Bytes (%d MB). Bitmap @ %018p\n",
        endOfMemory, ByteToMb(endOfMemory), BitmapAddress);

    return TRUE;
}
