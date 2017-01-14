#include "defs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "mem.h"
#include "log.h"
#include "physmemmgr.h"
#include "virtmemmgr.h"
#include "kpool.h"
#include "debugger.h"

#define PTE_COUNT               512
#define PTE_RECURSIVE_INDEX     511ULL
#define PML4_INDEX(x)           (((x) >> PML4_IDX_SHIFT) & 0x1FF)
#define PDP_INDEX(x)            (((x) >> PDP_IDX_SHIFT) & 0x1FF)
#define PD_INDEX(x)             (((x) >> PD_IDX_SHIFT) & 0x1FF)
#define PT_INDEX(x)             (((x) >> PT_IDX_SHIFT) & 0x1FF)

#define VAS_VMGR_PT             (0xFFFF000000000000ULL  + (PTE_RECURSIVE_INDEX << 39))
#define VAS_VMGR_PD             (VAS_VMGR_PT            + (PTE_RECURSIVE_INDEX << 30))
#define VAS_VMGR_PDP            (VAS_VMGR_PD            + (PTE_RECURSIVE_INDEX << 21))
#define VAS_VMGR_PML4           (VAS_VMGR_PDP           + (PTE_RECURSIVE_INDEX << 12))

// Note that if PTE_RECURSIVE_INDEX value changes from 511, the values hardcoded below must also be changed
static_assert(0xfffffffffffff000 == VAS_VMGR_PML4, "Unexpected PML4");
static_assert(0xffffffffffe00000 == VAS_VMGR_PDP, "Unexpected PDP");
static_assert(0xffffffffc0000000 == VAS_VMGR_PD, "Unexpected PD");
static_assert(0xffffff8000000000 == VAS_VMGR_PT, "Unexpected PT");

#define VA2PML4(vaddr)          (VAS_VMGR_PML4)
#define VA2PDP(vaddr)           (VAS_VMGR_PDP + (((vaddr) >> 27) & 0x00001FF000))
#define VA2PD(vaddr)            (VAS_VMGR_PD + (((vaddr) >> 18) & 0x003FFFF000))
#define VA2PT(vaddr)            (VAS_VMGR_PT + (((vaddr) >> 9) & 0x7FFFFFF000))

#define VAS_MAX_SIZE            (ONE_TB)
#define VAS_KERNEL              (ONE_TB)
#define VAS_LOWMEM              (0ULL)
#define VAS_STACK               (ONE_TB * 2)
#define VAS_POOL                (ONE_TB * 3)
#define VAS_POOL_SIZE           (32 * ONE_MB)
#define VAS_ONDEMAND            (ONE_TB * 4)
#define VAS_ONDEMAND_SIZE       (4 * ONE_MB)

typedef QWORD       PTE, *PPTE;

#pragma pack(push)
#pragma pack(1)

typedef struct _PT
{
    PTE     Entries[PTE_COUNT];
} PT, *PPT;

#pragma pack(pop)

#define PAGES_TO_PTES(pages)    (ROUND_UP((pages), PTE_COUNT) / PTE_COUNT)

static_assert(sizeof(PT) == PAGE_SIZE_4K, "PT size not 4K!");

typedef enum _TABLE_LEVEL
{
    levelPt = 1,
    levelPd,
    levelPdp,
    levelPml4
} TABLE_LEVEL;


static QWORD gVirtStackBase;
static QWORD gVirtStackTop;
static QWORD gNextStackBase;


QWORD
MmStckMoveBspStackAndAdjustRsp(
    __in QWORD NewStackTop
);


static 
NTSTATUS
_MmPhase1GetNextTable(
    _In_ PPT CurrentTable,
    _In_ WORD Index,
    _Out_ PPT *NextTable
)
{
    PTE pte = CurrentTable->Entries[Index];

    // not present? create one entry
    if (!(pte & PTE_P))
    {
        QWORD pa = 0;
        NTSTATUS status = MmAllocPhysicalPage(&pa);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        memset((VOID *)pa, 0, PAGE_SIZE_4K);
        CurrentTable->Entries[Index] = CLEAN_PHYADDR(pa) | PTE_P | PTE_RW | PTE_US;
    }

    // this works only while we are in Phase1 when we use the one-to-one paging tables
    *NextTable = (PT *)(CLEAN_PHYADDR(CurrentTable->Entries[Index]));

    return STATUS_SUCCESS;
}


static
NTSTATUS
_MmPhase1MapContigousRegion(
    _In_ QWORD FinalPdbr,
    _In_ QWORD PhysicalBase,
    _In_ QWORD VirtualBase,
    _In_ QWORD Size
)
{
    PPT pPml4 = (PT *)FinalPdbr;
    QWORD pages = SMALL_PAGE_COUNT(Size);
    QWORD nextPa = PhysicalBase;
    QWORD nextVa = VirtualBase;

    for (QWORD i = 0; i < pages; i++)
    {
        NTSTATUS status;
        PPT pPdp = NULL;
        PPT pPd = NULL;
        PPT pPt = NULL;

        // PML4E -> PDP
        status = _MmPhase1GetNextTable(pPml4, PML4_INDEX(nextVa), &pPdp);
        if (!NT_SUCCESS(status))
        {
            LogWithInfo("[ERROR] _MmPhase1GetNextTable failed: 0x%08x\n", status);
            return status;
        }

        // PDPE -> PD
        status = _MmPhase1GetNextTable(pPdp, PDP_INDEX(nextVa), &pPd);
        if (!NT_SUCCESS(status))
        {
            LogWithInfo("[ERROR] _MmPhase1GetNextTable failed: 0x%08x\n", status);
            return status;
        }

        // PDE -> PT
        status = _MmPhase1GetNextTable(pPd, PD_INDEX(nextVa), &pPt);
        if (!NT_SUCCESS(status))
        {
            LogWithInfo("[ERROR] _MmPhase1GetNextTable failed: 0x%08x\n", status);
            return status;
        }

        if (0 != (pPt->Entries[PT_INDEX(nextVa)] & PTE_P))
        {
            LogWithInfo("[WARNING] PTE for VA %018p is already set to %018p!\n", nextVa, pPt->Entries[PT_INDEX(nextVa)]);
        }

        pPt->Entries[PT_INDEX(nextVa)] = CLEAN_PHYADDR(nextPa) | PTE_P | PTE_RW | PTE_US;

        // next virtual and physical page
        nextVa += PAGE_SIZE_4K;
        nextPa += PAGE_SIZE_4K;
    }

    return STATUS_SUCCESS;
}


static
BOOLEAN
_MmIsVaRangeFree(
    _In_ QWORD Start,
    _In_ QWORD Length
)
{
    QWORD va = ROUND_DOWN(Start, PAGE_SIZE_4K);
    QWORD vaEnd = va + ROUND_UP(Length, PAGE_SIZE_4K);

    while (va < vaEnd)
    {
        PPT pTable = (PT *)VA2PML4(va);
        PTE pte = pTable->Entries[PML4_INDEX(va)];

        if (0 == (pte & PML4E_P))
        {
            // the 512G range described by this PML4E is free
            va = ROUND_DOWN(va, PAGE_SIZE_1G * 512) + PAGE_SIZE_1G * 512;
            continue;
        }

        pTable = (PT *)VA2PDP(va);
        pte = pTable->Entries[PDP_INDEX(va)];

        if (0 == (pte & PDPE_P))
        {
            // the entire 1G range described by this PML4E is free
            va = ROUND_DOWN(va, PAGE_SIZE_1G) + PAGE_SIZE_1G;
            continue;
        }

        if (0 != (pte & PDPE_PS))
        {
            // this is a 1G page
            return FALSE;
        }

        pTable = (PT *)VA2PD(va);
        pte = pTable->Entries[PD_INDEX(va)];

        if (0 == (pte & PDE_P))
        {
            // the entire 2M range described by this PML4E is free
            va = ROUND_DOWN(va, PAGE_SIZE_2M) + PAGE_SIZE_2M;
            continue;
        }

        if (0 != (pte & PDE_PS))
        {
            // this is a 2M page
            return FALSE;
        }

        pTable = (PT *)VA2PT(va);
        pte = pTable->Entries[PT_INDEX(va)];

        if (0 == (pte & PTE_P))
        {
            // free 4K page
            va += PAGE_SIZE_4K;
            continue;
        }

        if (0 != (pte & PDE_PS))
        {
            // this is a 4K page
            return FALSE;
        }
    }

    return TRUE;
}


static
NTSTATUS
_MmPreAllocVas(
    _In_opt_ PCHAR Name,
    _In_ QWORD Base,
    _In_ DWORD Length,
    _In_ WORD Attributes,
    _In_ BOOLEAN Empty
)
{
    DWORD pteCount = SMALL_PAGE_COUNT(Length);
    QWORD nextVa = Base;

    LogWithInfo("[VIRTMEM] Initializing VAS %s = [%018p, %018p) using %d 4K pages\n",
        Name ? Name : "", Base, Base + Length, pteCount);

    for (DWORD p = 0; p < pteCount; p++)
    {
        PPT pPml4 = (PT *)VA2PML4(nextVa);
        PPT pPdp = (PT *)VA2PDP(nextVa);
        PPT pPd = (PT *)VA2PD(nextVa);
        PPT pPt = (PT *)VA2PT(nextVa);
        PTE pte;

        pte = pPml4->Entries[PML4_INDEX(nextVa)];
        // no PDP, create one
        if (0 == (pte & PTE_P))
        {
            QWORD pa = 0;
            NTSTATUS status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            pPml4->Entries[PML4_INDEX(nextVa)] = CLEAN_PHYADDR(pa) | PTE_P | PTE_RW | PTE_US;
            memset(pPdp, 0, sizeof(PT));
        }

        // no Pd, create one
        pte = pPdp->Entries[PDP_INDEX(nextVa)];
        if (0 == (pte & PTE_P))
        {
            QWORD pa = 0;
            NTSTATUS status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            pPdp->Entries[PDP_INDEX(nextVa)] = CLEAN_PHYADDR(pa) | PTE_P | PTE_RW | PTE_US;
            memset(pPd, 0, sizeof(PT));
        }

        pte = pPd->Entries[PD_INDEX(nextVa)];
        // no PT, create one
        if (0 == (pte & PTE_P))
        {
            QWORD pa = 0;
            NTSTATUS status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            pPd->Entries[PD_INDEX(nextVa)] = CLEAN_PHYADDR(pa) | PTE_P | PTE_RW | PTE_US;
            memset(pPt, 0, sizeof(PT));
        }

        pte = pPt->Entries[PT_INDEX(nextVa)];

        if (0 != (pte & PTE_P))
        {
            LogWithInfo("[FATAL ERROR] VA %018p is already reserved. PTE = %018p\n", nextVa, pte);
            return STATUS_INTERNAL_ERROR;
        }
        else
        {
            if (!Empty)
            {
                QWORD pa = 0;
                NTSTATUS status = MmAllocPhysicalPage(&pa);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                pPt->Entries[PT_INDEX(nextVa)] = CLEAN_PHYADDR(pa) | Attributes | PTE_P;
            }
            else
            {
                pPt->Entries[PT_INDEX(nextVa)] = 0ULL;
            }
        }

        nextVa += PAGE_SIZE_4K;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MmMapContigousPhysicalRegion(
    _In_ QWORD PhysicalBase,
    _In_ QWORD VirtualBase,
    _In_ QWORD Size
)
{
    QWORD pages = SMALL_PAGE_COUNT(Size);
    QWORD nextVa = VirtualBase;
    QWORD nextPa = PhysicalBase;
    NTSTATUS status;
    PPT pPml4;
    PPT pPdp;
    PPT pPd;
    PPT pPt;

    if (PhysicalBase % PAGE_SIZE_4K)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (VirtualBase % PAGE_SIZE_4K)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    for (QWORD count = 0; count < pages; count++)
    {
        pPml4 = (PT *)VA2PML4(nextVa);

        if (!(pPml4->Entries[PML4_INDEX(nextVa)] & PML4E_P))
        {
            QWORD pa = 0;
            status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                LogWithInfo("[ERROR] MmAllocPhysicalPage failed: 0x%08x\n", status);
                return status;
            }

            memset(pPml4, 0, sizeof(PT));
            pPml4->Entries[PML4_INDEX(nextVa)] = CLEAN_PHYADDR(pa) | PTE_P | PTE_US | PTE_RW;
        }

        pPdp = (PT *)VA2PDP(nextVa);

        if (!(pPdp->Entries[PDP_INDEX(nextVa)] & PDPE_P))
        {
            QWORD pa = 0;
            status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                LogWithInfo("[ERROR] MmAllocPhysicalPage failed: 0x%08x\n", status);
                return status;
            }

            memset(pPdp, 0, sizeof(PT));
            pPdp->Entries[PDP_INDEX(nextVa)] = CLEAN_PHYADDR(pa) | PTE_P | PTE_US | PTE_RW;
        }

        pPd = (PT *)VA2PD(nextVa);

        if (!(pPd->Entries[PD_INDEX(nextVa)] & PDE_P))
        {
            QWORD pa = 0;
            status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                LogWithInfo("[ERROR] MmAllocPhysicalPage failed: 0x%08x\n", status);
                return status;
            }

            memset(pPd, 0, sizeof(PT));
            pPd->Entries[PD_INDEX(nextVa)] = CLEAN_PHYADDR(pa) | PTE_P | PTE_US | PTE_RW;
        }

        pPt = (PT *)VA2PT(nextVa);

        if (0 != (pPt->Entries[PT_INDEX(nextVa)] & PTE_P))
        {
            LogWithInfo("[WARNING] Overwriting PTE %018p for VA %018p!\n", pPt->Entries[PT_INDEX(nextVa)], nextVa);
        }

        pPt->Entries[PT_INDEX(nextVa)] = CLEAN_PHYADDR(nextPa) | PTE_P | PTE_US | PTE_RW;

        nextVa += PAGE_SIZE_4K;
        nextPa += PAGE_SIZE_4K;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MmMapVaToPa(
    _In_ QWORD PhysicalFrame,
    _In_ QWORD VirtualAddress,
    _In_ BOOLEAN LargePage,
    _In_ WORD Attributes
)
{
    PPT pTable;
    PTE pte;

    if ((LargePage && (PhysicalFrame % PAGE_SIZE_2M)) || (PhysicalFrame % PAGE_SIZE_4K))
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if ((LargePage && (VirtualAddress % PAGE_SIZE_2M)) || (VirtualAddress % PAGE_SIZE_4K))
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    pTable = (PT *)VA2PML4(VirtualAddress);             // PML4
    pte = pTable->Entries[PML4_INDEX(VirtualAddress)];  // PML4E
    if (0 == (pte & PML4E_P))
    {
        // the needed PDP is not present, create one
        QWORD pa = 0;
        NTSTATUS status = MmAllocPhysicalPage(&pa);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        pTable->Entries[PML4_INDEX(VirtualAddress)] = CLEAN_PHYADDR(pa) | PML4E_P | PML4E_RW | PML4E_US;
        pTable = (PT *)VA2PDP(VirtualAddress);
        memset(pTable, 0, sizeof(PT));
    }
    else
    {
        // present, simply advance the table
        pTable = (PT *)VA2PDP(VirtualAddress);
    }

    // pTable is now the required PDP
    pte = pTable->Entries[PDP_INDEX(VirtualAddress)];   // PDPE
    if (0 == (pte & PDPE_P))
    {
        // the needed PD is not present, create one
        QWORD pa = 0;
        NTSTATUS status = MmAllocPhysicalPage(&pa);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        pTable->Entries[PDP_INDEX(VirtualAddress)] = CLEAN_PHYADDR(pa) | PDPE_P | PDPE_RW | PDPE_US;
        pTable = (PT *)VA2PD(VirtualAddress);
        memset(pTable, 0, sizeof(PT));
    }
    else
    {
        // present, simply advance the table
        pTable = (PT *)VA2PD(VirtualAddress);
    }

    // pTable is now the required PD
    pte = pTable->Entries[PD_INDEX(VirtualAddress)];    // PDE
    if (LargePage)
    {
        // 2M page, add a PDE and return
        if (0 == (pte & PDE_P))
        {
            pTable->Entries[PD_INDEX(VirtualAddress)] = CLEAN_PHYADDR(PhysicalFrame) | PDE_PS | Attributes;
            return STATUS_SUCCESS;
        }
        else
        {
            return STATUS_PAGE_ALREADY_RESERVED;
        }
    }
    else
    {
        // 4K page, get the PT
        if (0 == (pte & PDE_P))
        {
            // the needed PT is not present, create one
            QWORD pa = 0;
            NTSTATUS status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            pTable->Entries[PD_INDEX(VirtualAddress)] = CLEAN_PHYADDR(pa) | PDE_P | PDE_RW | PDE_US;
            pTable = (PT *)VA2PT(VirtualAddress);
            memset(pTable, 0, sizeof(PT));
        }
        else
        {
            // present, simply advance the table
            pTable = (PT *)VA2PT(VirtualAddress);
        }

        pte = pTable->Entries[PT_INDEX(VirtualAddress)];
        if (0 == (pte & PDE_P))
        {
            pTable->Entries[PT_INDEX(VirtualAddress)] = CLEAN_PHYADDR(PhysicalFrame) | Attributes;
            return STATUS_SUCCESS;
        }
        else
        {
            return STATUS_PAGE_ALREADY_RESERVED;
        }
    }
}


NTSTATUS
MmStackAlloc(
    _In_ DWORD Size,
    _Out_ QWORD *StackTop
)
{
    DWORD pages = SMALL_PAGE_COUNT(Size);

    if (Size % PAGE_SIZE_4K)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!StackTop)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (gNextStackBase + Size >= gVirtStackTop)
    {
        return STATUS_NO_MEMORY;
    }

    for (DWORD i = 0; i < pages; i++)
    {
        QWORD pa = 0;
        NTSTATUS status = MmAllocPhysicalPage(&pa);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = MmMapVaToPa(pa, gNextStackBase, FALSE, PTE_P | PTE_US | PTE_RW);
        if (!NT_SUCCESS(status))
        {
            LogWithInfo("[ERROR] MmMapVaToPa failed for %018p -> %018p: 0x%08x\n", pa, gNextStackBase, status);
            return status;
        }

        gNextStackBase += PAGE_SIZE_4K;
    }

    *StackTop = gNextStackBase - PAGE_SIZE_4K;

    return STATUS_SUCCESS;
}


NTSTATUS
MmVirtualManagerInit(
    _In_ QWORD MaximumMemorySize,
    _In_ QWORD KernelPaStart,
    _In_ QWORD KernelVaStart,
    _In_ QWORD KernelRegionLength
)
{
    QWORD pteCount;
    QWORD pdeCount;
    QWORD pdpeCount;
    QWORD pml4eCount;
    QWORD pagesNeeded;
    PPT pPml4;
    QWORD pdbr;
    NTSTATUS status;
    QWORD rsp;
    QWORD magic;

    pteCount = SMALL_PAGE_COUNT(MaximumMemorySize); // how many pages we have
    pdeCount = PAGES_TO_PTES(pteCount);             // how many PTs we need
    pdpeCount = PAGES_TO_PTES(pdeCount);            // how many PDs we need
    pml4eCount = PAGES_TO_PTES(pdpeCount);          // how many PDPs we need

    if (pml4eCount > PTE_COUNT)
    {
        LogWithInfo("Too many PML4Es needed for 0x%llx bytes (%d MB): %d\n", 
            pml4eCount, MaximumMemorySize, ByteToMb(MaximumMemorySize));
        return STATUS_NOT_SUPPORTED;
    }

    pagesNeeded = 1 + pml4eCount + pdpeCount + pdeCount;

    //Log("[VIRTMEM] In order to map 0x%llx bytes (%d MB) we need:\n", MaximumMemorySize, ByteToMb(MaximumMemorySize));
    //Log("\t%d PML4 Entries, %d PDP Entries, %d PD Entries, %d PT Entries\n",
    //    pml4eCount, pdpeCount, pdeCount, pteCount);
    //Log("\t%d pages (%d MB)\n", pagesNeeded, ByteToMb(pagesNeeded * PAGE_SIZE_4K));

    pdbr = 0;
    status = MmAllocPhysicalPage(&pdbr);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] MmAllocPhysicalPage failed: 0x%08x\n", status);
        return status;
    }

    pPml4 = (PT *)pdbr;
    Log("[VIRTMEM] PDBR @ %018p (PA)\n", pdbr);
    memset(pPml4->Entries, 0, sizeof(pPml4->Entries));

    // install the recursive entry
    pPml4->Entries[PTE_RECURSIVE_INDEX] = CLEAN_PHYADDR(pdbr) | PML4E_P | PML4E_RW | PML4E_US;

    // also make the current PDBR recursive
    pPml4 = (PT *)CLEAN_PHYADDR(__readcr3());
    pPml4->Entries[PTE_RECURSIVE_INDEX] = CLEAN_PHYADDR(__readcr3()) | PML4E_P | PML4E_RW | PML4E_US;

    // map the kernel
    status = _MmPhase1MapContigousRegion(pdbr, KernelPaStart, KernelVaStart, KernelRegionLength);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] _MmPhase1MapContigousRegion failed: 0x%08x\n", status);
        return status;
    }

    Log("[VIRTMEM] Mapped the kernel: [%018p, %018p) -> [%018p, %018p)\n",
        KernelPaStart, KernelPaStart + KernelRegionLength, KernelVaStart, KernelVaStart + KernelRegionLength);

    // map low memory (4K - 2M)
    status = _MmPhase1MapContigousRegion(pdbr, ONE_KB * 4, ONE_KB * 4, 2 * ONE_MB - ONE_KB * 4);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] _MmPhase1MapContigousRegion failed: 0x%08x\n", status);
        return status;
    }

    // switch to the final PDBR
    Log("[VIRTMEM] Switching PDBR from %018p to %018p...\n", __readcr3(), pdbr);
    __writecr3(pdbr);
    Log("[VIRTMEM] PDBR switched to: %018p\n", __readcr3());

    // init the stack VAS
    gVirtStackBase = VAS_STACK;
    gVirtStackTop = VAS_STACK + ONE_TB;
    gNextStackBase = gVirtStackBase;

    rsp = 0;
    status = MmStackAlloc(PAGE_SIZE_2M, &rsp);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[FATAL ERROR] MmStackAlloc failed: 0x%08x\n", status);
        return status;
    }

    Log("Switching BSP stack to %018p\n", rsp);
    magic = MmStckMoveBspStackAndAdjustRsp(rsp - sizeof(PVOID));
    Log("Magic: %018p\n", magic);

    // init all the VAS
    status = _MmPreAllocVas("POOL", VAS_POOL, VAS_POOL_SIZE, PTE_P | PTE_RW | PTE_US, FALSE);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] _MmPreAllocVas failed for %018p: 0x%08x\n", VAS_POOL, status);
        return status;
    }

    status = _MmPreAllocVas("ONDEMAND", VAS_ONDEMAND, VAS_ONDEMAND_SIZE, PTE_P | PTE_RW | PTE_US, TRUE);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] _MmPreAllocVas failed for %018p: 0x%08x\n", VAS_ONDEMAND, status);
        return status;
    }

    // init the kernel pool allocator
    status = KpInit((VOID *)VAS_POOL, 32 * ONE_MB, PAGE_SIZE_4K);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] KpInit failed: 0x%08x\n", status);
        return status;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MmTranslateVa(
    _In_ PVOID Va,
    _Out_ QWORD *Pa,
    _Out_ DWORD *PageSize
)
{
    PPT pTable = (PT *)VA2PML4(Va);
    PTE pte = pTable->Entries[PML4_INDEX((QWORD)Va)];

    if (0 == (pte & PML4E_P))
    {
        return STATUS_UNSUCCESSFUL;
    }

    pTable = (PT *)VA2PDP((QWORD)Va);
    pte = pTable->Entries[PDP_INDEX((QWORD)Va)];

    if (0 == (pte & PDPE_P))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (0 != (pte & PDPE_PS))
    {
        *PageSize = PAGE_SIZE_1G;
        *Pa = (pte & ~OFFSET_1G_MASK) | ((QWORD)Va & OFFSET_1G_MASK);
        return STATUS_SUCCESS;
    }

    pTable = (PT *)VA2PD((QWORD)Va);
    pte = pTable->Entries[PD_INDEX((QWORD)Va)];

    if (0 == (pte & PDE_P))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (0 != (pte & PDE_PS))
    {
        *PageSize = PAGE_SIZE_2M;
        *Pa = (pte & ~OFFSET_2M_MASK) | ((QWORD)Va & OFFSET_2M_MASK);
        return STATUS_SUCCESS;
    }

    pTable = (PT *)VA2PT((QWORD)Va);
    pte = pTable->Entries[PT_INDEX((QWORD)Va)];

    if (0 == (pte & PTE_P))
    {
        return STATUS_UNSUCCESSFUL;
    }

    *PageSize = PAGE_SIZE_4K;
    *Pa = (pte & ~OFFSET_4K_MASK) | ((QWORD)Va & OFFSET_4K_MASK);

    return STATUS_SUCCESS;
}


static
NTSTATUS
_MmGetFirstFreePteInVas(
    _In_ QWORD VasBase,
    _In_ DWORD VasLength,
    _Out_ PPT *FreePt,
    _Out_ WORD *FreeIndex,
    _Out_opt_ QWORD *ReservedVa
)
{
    const QWORD pages = SMALL_PAGE_COUNT(VasLength);
    QWORD nextVa = VasBase;

    for (QWORD p = 0; p < pages; p++)
    {
        PPT pTable;
        PTE pte;
        WORD idx;

        // get PML4
        pTable = (PT *)VA2PML4(nextVa);
        idx = PML4_INDEX(nextVa);
        pte = pTable->Entries[idx];
        if (0 == (pte & PML4E_P))
        {
            QWORD pa = 0;
            NTSTATUS status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            pTable->Entries[idx] = CLEAN_PHYADDR(pa) | PML4E_P | PML4E_RW | PML4E_US;
        }

        // get PDP
        pTable = (PT *)VA2PDP(nextVa);
        idx = PDP_INDEX(nextVa);
        pte = pTable->Entries[idx];
        if (0 == (pte & PDPE_P))
        {
            QWORD pa = 0;
            NTSTATUS status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            pTable->Entries[idx] = CLEAN_PHYADDR(pa) | PDPE_P | PDPE_RW | PDPE_US;
        }

        // get PD
        pTable = (PT *)VA2PD(nextVa);
        idx = PD_INDEX(nextVa);
        pte = pTable->Entries[idx];
        if (0 == (pte & PDE_P))
        {
            QWORD pa = 0;
            NTSTATUS status = MmAllocPhysicalPage(&pa);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            pTable->Entries[idx] = CLEAN_PHYADDR(pa) | PDE_P | PDE_RW | PDE_US;
        }

        // get PT
        pTable = (PT *)VA2PT(nextVa);
        idx = PT_INDEX(nextVa);
        pte = pTable->Entries[idx];
        if (0 == (pte & PTE_P))
        {
            // found it!
            *FreePt = pTable;
            *FreeIndex = idx;

            if (ReservedVa)
            {
                *ReservedVa = nextVa;
            }

            return STATUS_SUCCESS;
        }

        nextVa += PAGE_SIZE_4K;
    }

    return STATUS_NOT_FOUND;
}


static
NTSTATUS
_MmGetFreeRangeInVas(
    _In_ QWORD VasBase,
    _In_ QWORD VasLength,
    _In_ DWORD RangeLength,
    _Out_ QWORD *FirstFreeVa
)
{
    QWORD vasPages = SMALL_PAGE_COUNT(VasLength);
    QWORD neededPages = SMALL_PAGE_COUNT(RangeLength);
    QWORD pagesLeft = neededPages;
    QWORD va = VasBase;
    QWORD start = 0;

    for (QWORD p = 0; p < vasPages && 0 != pagesLeft; p++)
    {
        PPT pTable;
        PTE pte;

        // check PML4E
        pTable = (PT *)VA2PML4(va);
        pte = pTable->Entries[PML4_INDEX(va)];
        if (0 == (pte & PTE_P))
        {
            pagesLeft = neededPages;
            start = 0;
            goto _next;
        }

        // check PDPE
        pTable = (PT *)VA2PDP(va);
        pte = pTable->Entries[PDP_INDEX(va)];
        if (0 == (pte & PTE_P))
        {
            pagesLeft = neededPages;
            start = 0;
            goto _next;
        }

        // check PDE
        pTable = (PT *)VA2PD(va);
        pte = pTable->Entries[PD_INDEX(va)];
        if (0 == (pte & PTE_P))
        {
            pagesLeft = neededPages;
            start = 0;
            goto _next;
        }

        // check PTE
        pTable = (PT *)VA2PT(va);
        pte = pTable->Entries[PT_INDEX(va)];
        if (0 == (pte & PTE_P))
        {
            if (!start)
            {
                start = va;
            }
            pagesLeft--;
        }

    _next:
        va += PAGE_SIZE_4K;
    }

    if (!pagesLeft && va)
    {
        *FirstFreeVa = va;
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}



NTSTATUS
MmMapPhysicalPages(
    _In_ QWORD PhysicalBase,
    _In_ DWORD RangeSize,
    _Out_ PVOID *Ptr,
    _In_ DWORD Flags
)
{
    NTSTATUS status;
    QWORD vaStart = 0;
    DWORD rangeSize = ROUND_UP(RangeSize, PAGE_SIZE_4K);

    if (!Ptr)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    if (0 == (MAP_FLG_SKIP_PHYPAGE_CHECK & Flags))
    {
        status = MmReservePhysicalRange(PhysicalBase, rangeSize);
        if (!NT_SUCCESS(status))
        {
            LogWithInfo("[ERROR] MmReservePhysicalRange failed for [%018p, %18p): 0x%08x\n", 
                PhysicalBase, PhysicalBase + rangeSize, status);
            return status;
        }
    }

    status = _MmGetFreeRangeInVas(VAS_ONDEMAND, VAS_ONDEMAND_SIZE, rangeSize, &vaStart);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] _MmGetFreeRangeInVas failed: 0x%08x\n", status);
        goto _cleanup_and_exit;
    }

    status = MmMapContigousPhysicalRegion(PhysicalBase, vaStart, rangeSize);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] MmMapContigousPhysicalRegion failed for [%018p, %018p) -> [%018p, %018p): 0x%08x\n",
            PhysicalBase, PhysicalBase + rangeSize, vaStart, vaStart + rangeSize, status);
        goto _cleanup_and_exit;
    }

    status = STATUS_SUCCESS;
    *Ptr = (VOID *)vaStart;

_cleanup_and_exit:
    if (!NT_SUCCESS(status))
    {
        if (0 == (MAP_FLG_SKIP_PHYPAGE_CHECK & Flags))
        {
            for (QWORD i = 0; i < rangeSize; i += PAGE_SIZE_4K)
            {
                MmFreePhysicalPage(i + PhysicalBase);
            }
        }
    }

    return status;
}


NTSTATUS
MmUnmapRangeAndNull(
    _Inout_ PVOID *Ptr,
    _In_ DWORD Length,
    _In_ DWORD Flags
)
{
    QWORD pages;

    if (!Ptr || !*Ptr)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    Length = ROUND_UP(Length, PAGE_SIZE_4K);
    pages = SMALL_PAGE_COUNT(Length);

    for (QWORD p = 0; p < pages; p++)
    {
        QWORD va = (QWORD)(*Ptr) + p * PAGE_SIZE_4K;
        PPT pPt = (PT *)VA2PT(va);
        WORD idx = PT_INDEX(va);

        if (0 == (MAP_FLG_SKIP_PHYPAGE_CHECK & Flags))
        {
            QWORD pa = CLEAN_PHYADDR(pPt->Entries[idx]);
            MmFreePhysicalPage(pa);
        }

        pPt->Entries[idx] = 0ULL;

        __invlpg(va);
    }

    return STATUS_SUCCESS;
}


VOID
MmDumpVas(
    _In_ QWORD VaBase,
    _In_ QWORD Length
)
{
    QWORD pages = SMALL_PAGE_COUNT(Length);

    for (QWORD p = 0; p < pages; p++)
    {
        QWORD va = VaBase + p * PAGE_SIZE_4K;
        PPT pPml4 = (PT *)VA2PML4(va);
        PPT pPdp = (PT *)VA2PDP(va);
        PPT pPd = (PT *)VA2PD(va);
        PPT pPt = (PT *)VA2PT(va);

        LogWithInfo("VA %018p -> PXE @ %018p, PDP @ %018p, PD @ %018p, PT @ %018p\n",
            va, pPml4, pPdp, pPd, pPt);
        LogWithInfo("\t\t PXE[%d] = %018p, PDP[%d] = %018p, PD[%d] = %018p, PT[%d] = %018p\n",
            PML4_INDEX(va), pPml4->Entries[PML4_INDEX(va)], 
            PDP_INDEX(va), pPdp->Entries[PDP_INDEX(va)],
            PD_INDEX(va), pPd->Entries[PD_INDEX(va)], 
            PT_INDEX(va), pPt->Entries[PT_INDEX(va)]);
    }
}
