#include "defs.h"
#include "memdefs.h"
#include "memmap.h"
#include "physmemmgr.h"
#include "log.h"

extern MMAP_ENTRY gBootMemoryMap[MAX_MMAP_ENTRIES];
extern DWORD gBootMemoryMapEntries;
extern SIZE_T gBootMemoryLimit;

typedef struct _PHYSMEM_STATE
{
    PQWORD      Bitmap;     // we use PQWORD because it will be faster to do some checks

    DWORD       PageSize;   // one bit in the bitmap describes one page of this size
    DWORD       PageCount;  // how many bits we have; Bitmap will contain PageCount/sizeof(Bitmap[0]) entries
    DWORD       FreePages;  // number of current free pages
} PHYSMEM_STATE, *PPHYSMEM_STATE;

static PHYSMEM_STATE gPhysMemState;

#define BITS_PER_ENTRY      (sizeof(QWORD) * 8)

static __forceinline
BOOLEAN
_MmIsBitSet(
    _In_ SIZE_T Bit
)
{
    return 0 != (gPhysMemState.Bitmap[Bit / BITS_PER_ENTRY] & (1ULL << (Bit % BITS_PER_ENTRY)));
}


static __forceinline
VOID
_MmSetBit(
    _In_ SIZE_T Bit
)
{
    gPhysMemState.Bitmap[Bit / BITS_PER_ENTRY] |= (1ULL << (Bit % BITS_PER_ENTRY));
}


static __forceinline
VOID
_MmClearBit(
    _In_ SIZE_T Bit
)
{
    gPhysMemState.Bitmap[Bit / BITS_PER_ENTRY] &= ~(1ULL << (Bit % BITS_PER_ENTRY));
}


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

    memset(&gPhysMemState, 0, sizeof(gPhysMemState));
    gPhysMemState.Bitmap = BitmapAddress;
    gPhysMemState.PageSize = PAGE_SIZE_4K;
    // safe cast
    gPhysMemState.PageCount = (DWORD)(endOfMemory / gPhysMemState.PageSize);

    Log("[PHYSMEM] Page count: %d\n", gPhysMemState.PageCount);

    // reserve every page for now
    memset(gPhysMemState.Bitmap, 0xFF, gPhysMemState.PageCount);
    gPhysMemState.FreePages = 0;

    return TRUE;
}


//QWORD
//MmGetFreePhysicalRegion(
//    _In_ DWORD PageCount
//)
//{
//    DWORD start = 0;
//    DWORD end = 0;
//    DWORD foundPages = 0;
//    BOOLEAN bStartValid = FALSE;
//    BOOLEAN bEndValid = FALSE;
//
//    // simple check
//    if (PageCount >= gPhysMemState.FreePages)
//    {
//        LogWithInfo("[ERROR] Requested page count is too high: %d\n", PageCount);
//        return (QWORD)-1;
//    }
//
//    for (DWORD q = 0; q < gPhysMemState.PageCount / BITS_PER_ENTRY; q++)
//    {
//        // if the entire QWORD is set, these 64 pages are already reserved and there is no point in checking them
//        if ((QWORD)-1 == gPhysMemState.Bitmap[q])
//        {
//            foundPages = 0;  // also, reset the found count
//            bStartValid = FALSE;
//            bEndValid = FALSE;
//            continue;
//        }
//
//        // test each page in the current 64 pages
//        for (BYTE p = 0; p < BITS_PER_ENTRY; p++)
//        {
//            // bit not set => free page
//            if (!_MmIsBitSet(p + q * BITS_PER_ENTRY))
//            {
//                foundPages++;
//
//                if (!bStartValid)
//                {
//                    start = p + q * BITS_PER_ENTRY;
//                    bStartValid = TRUE;
//                }
//                else if (!bEndValid)
//                {
//                    end = p + q * BITS_PER_ENTRY;
//                    bEndValid = TRUE;
//                }
//                else
//                {
//                    // wtf?
//                    LogWithInfo("[WARNING] Valid start (0x%08x) and end (0x%08x) with page count: %d/%d\n", start, end, foundPages, PageCount);
//                    break;
//                }
//            }
//            // reset everything
//            else
//            {
//                bStartValid = FALSE;
//                bEndValid = FALSE;
//                start = (QWORD)-1;
//                end = (QWORD)-1;
//                foundPages = 0;
//            }
//
//            // found all the pages we needed
//            if (foundPages == PageCount)
//            {
//                break;
//            }
//        }
//    }
//
//    if (foundPages == PageCount)
//    {
//        return start;
//    }
//
//    return (QWORD)-1;
//}
//
//
//QWORD
//MmAllocPhysicalRegion(
//    _In_ DWORD RegionSize
//)
//{
//    DWORD pages = RegionSize / gPhysMemState.PageSize;
//    QWORD start = 0;
//    QWORD end = 0;
//
//    if (pages % gPhysMemState.PageSize)
//    {
//        pages++;
//    }
//
//    start = MmGetFreePhysicalRegion((DWORD)pages);
//    if ((QWORD)-1 == start)
//    {
//        LogWithInfo("[ERROR] MmGetFreePhysicalRegion failed for %d\n", pages);
//        return (QWORD)-1;
//    }
//
//    end = start + pages;
//
//    // now simply reserve
//    for (QWORD i = start; i < end; i++)
//    {
//        _MmSetBit(i);
//    }
//
//    // return the first page in the region
//    return start * gPhysMemState.PageSize;
//}
