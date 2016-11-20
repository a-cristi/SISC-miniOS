#include "defs.h"
#include "physmemmgr.h"
#include "log.h"

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
    VOID
)
{
    if (!MmA20Enable())
    {
        Log("[FATAL ERROR] Failed to enable the A20 line!\n");
        return FALSE;
    }

    Log("A20 line is enabled.\n");

    return TRUE;
}