#include "defs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "memmap.h"
#include "physmemmgr.h"
#include "log.h"

/*

    Simple physical memory manager.
    A bitmap is used to describe the entire physical memory. One bit describes one memory page (default is 4K):
    - if set, the page is reserved
    - if cleared, the page is free
    For easier maintainability, the bitmap is seed as a QWORD array. This allows us to quickly check 64 pages at once.

    Internally, a page is converted to an index (for example, when using 4K pages, the page 0x1000 is converted to index 1).

*/

extern MMAP_ENTRY gBootMemoryMap[MAX_MMAP_ENTRIES];
extern DWORD gBootMemoryMapEntries;
extern SIZE_T gBootMemoryLimit;

typedef struct _PHYSMEM_STATE
{
    PQWORD      Bitmap;     // we use PQWORD because it will be faster to do some checks

    DWORD       PageSize;   // one bit in the bitmap describes one page of this size
    DWORD       PageCount;  // how many bits we have; Bitmap will contain PageCount/sizeof(Bitmap[0]) entries
    DWORD       FreePages;  // number of current free pages

    QWORD       EndOfMemory;
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


static
NTSTATUS
_MmGetFreePhysicalPageIndex(
    _Out_ QWORD * PageIndex
)
{
    for (DWORD q = 0; q < gPhysMemState.PageCount / BITS_PER_ENTRY; q++)
    {
        // if the entire QWORD is set it means that these 64 pages are already reserved
        if ((QWORD)-1 == gPhysMemState.Bitmap[q])
        {
            continue;
        }

        // test each page
        for (BYTE p = 0; p < BITS_PER_ENTRY; p++)
        {
            if (0 == (gPhysMemState.Bitmap[q] & (1ULL << p)))
            {
                // found it!
                *PageIndex = (q * BITS_PER_ENTRY + p);
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_NOT_FOUND;
}


static
NTSTATUS
_MmGetFreePhysicalRangeIndex(
    _In_ QWORD PageCount,
    _Out_ QWORD *StartIndex
)
{
    QWORD startIndex = 0;
    BOOLEAN bStartFound = FALSE;
    BOOLEAN bEndFound = FALSE;
    QWORD count = 0;

    for (DWORD q = 0; q < gPhysMemState.PageCount / BITS_PER_ENTRY; q++)
    {
        // if the entire QWORD is set it means that these 64 pages are already reserved
        if ((QWORD)-1 == gPhysMemState.Bitmap[q])
        {
            bStartFound = bEndFound = FALSE;
            count = 0;
            continue;
        }

        // test each page
        for (BYTE p = 0; p < BITS_PER_ENTRY; p++)
        {
            if (0 == (gPhysMemState.Bitmap[q] & (1ULL << p)))
            {
                if (!bStartFound)
                {
                    bStartFound = TRUE;
                    startIndex = q * BITS_PER_ENTRY + p;
                }
                else
                {
                    bEndFound = TRUE;
                }

                count++;
            }
            else
            {
                bStartFound = bEndFound = FALSE;
                count = 0;
            }

            if (count == PageCount)
            {
                *StartIndex = startIndex;
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_NOT_FOUND;
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
    gPhysMemState.EndOfMemory = endOfMemory;
    // safe cast
    gPhysMemState.PageCount = (DWORD)(endOfMemory / gPhysMemState.PageSize);

    if (gPhysMemState.PageCount > 4 * ONE_MB)
    {
        LogWithInfo("[ERROR] Not enough memory is available for the physical memory bitmap\n");
        return FALSE;
    }

    Log("[PHYSMEM] Page count: %d\n", gPhysMemState.PageCount);

    // reserve every page for now
    memset(gPhysMemState.Bitmap, 0xFF, gPhysMemState.PageCount);
    gPhysMemState.FreePages = 0;

    // free only the pages that are usable in the memory map
    for (DWORD i = 0; i < gBootMemoryMapEntries; i++)
    {
        if (memTypeUsable == gBootMemoryMap[i].Type)
        {
            QWORD start = ROUND_DOWN(gBootMemoryMap[i].Base, gPhysMemState.PageSize);
            QWORD end = start + ROUND_UP(gBootMemoryMap[i].Length, gPhysMemState.PageSize);

            while (start < end)
            {
                NTSTATUS status = MmFreePhysicalPage(start);
                if (!NT_SUCCESS(status))
                {
                    LogWithInfo("[ERROR] MmFreePhysicalPage failed for %018p: 0x%08x\n", start, status);
                    return FALSE;
                }

                start += gPhysMemState.PageSize;
            }
        }
    }

    return TRUE;
}


NTSTATUS
MmReservePhysicalPage(
    _In_ QWORD Page
)
{
    QWORD bit = 0;

    if (Page > gPhysMemState.EndOfMemory)
    {
        return STATUS_NOT_FOUND;
    }

    Page = PHYPAGE_ALIGN(Page);
    bit = Page / gPhysMemState.PageSize;

    if (_MmIsBitSet(bit))
    {
        return STATUS_PAGE_ALREADY_RESERVED;
    }

    _MmSetBit(bit);
    gPhysMemState.FreePages--;

    return STATUS_SUCCESS;
}


NTSTATUS
MmFreePhysicalPage(
    _In_ QWORD Page
)
{
    QWORD bit = 0;

    if (Page > gPhysMemState.EndOfMemory)
    {
        return STATUS_NOT_FOUND;
    }

    Page = PHYPAGE_ALIGN(Page);
    bit = Page / gPhysMemState.PageSize;

    if (!_MmIsBitSet(bit))
    {
        return STATUS_PAGE_ALREADY_FREE;
    }

    _MmClearBit(bit);
    gPhysMemState.FreePages++;

    return STATUS_SUCCESS;
}


NTSTATUS
MmAllocPhysicalPage(
    _Inout_ QWORD * Page
)
{
    NTSTATUS status;
    QWORD pageIndex = 0;

    if (!Page)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!gPhysMemState.FreePages)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = _MmGetFreePhysicalPageIndex(&pageIndex);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] _MmGetFreePhysicalPageIndex failed: 0x%08x\n", status);
        return status;
    }

    *Page = pageIndex * gPhysMemState.PageSize;

    return MmReservePhysicalPage(*Page);
}


/// not needed for now
//NTSTATUS
//MmGetPhysicalRange(
//    _In_ QWORD PageCount,
//    _Inout_ QWORD *RangeStart
//)
//{
//    NTSTATUS status;
//    QWORD startIndex = 0;
//
//    if (!RangeStart)
//    {
//        return STATUS_INVALID_PARAMETER_1;
//    }
//
//    if (gPhysMemState.FreePages < PageCount)
//    {
//        return STATUS_INSUFF_SERVER_RESOURCES;
//    }
//
//    status = _MmGetFreePhysicalRangeIndex(PageCount, &startIndex);
//    if (!NT_SUCCESS(status))
//    {
//        LogWithInfo("[ERROR] _MmGetFreePhysicalRangeIndex failed: 0x%08x\n", status);
//        return status;
//    }
//
//    *RangeStart = startIndex * gPhysMemState.PageSize;
//
//    for (QWORD i = 0; i < PageCount; i++)
//    {
//        status = MmReservePhysicalPage(i * gPhysMemState.PageSize + *RangeStart);
//        if (!NT_SUCCESS(status))
//        {
//            LogWithInfo("[ERROR] MmReservePhysicalPage failed for %018p: 0x%08x\n", 
//                i * gPhysMemState.PageSize + *RangeStart, status);
//
//            for (QWORD j = 0; j < i; j++)
//            {
//                MmFreePhysicalPage(j * gPhysMemState.PageSize + *RangeStart);
//            }
//
//            return status;
//        }
//    }
//
//    return STATUS_SUCCESS;
//}


QWORD
MmGetTotalFreeMemory(
    VOID
)
{
    return gPhysMemState.FreePages * gPhysMemState.PageSize;
}
