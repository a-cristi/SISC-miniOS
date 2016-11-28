#ifndef _MEMDEFS_H_
#define _MEMDEFS_H_

#define ONE_KB          (1024ULL * 1)
#define ONE_MB          (1024ULL * ONE_KB)
#define ONE_GB          (1024ULL * ONE_MB)
#define ONE_TB          (1024ULL * ONE_GB)

#define KbToByte(x)     ((x) * ONE_KB)
#define MbToByte(x)     ((x) * ONE_MB)
#define GbToByte(x)     ((x) * ONE_GB)
#define TbToByte(x)     ((x) * ONE_TB)

#define ByteToKb(x)     ((x) / ONE_KB)
#define ByteToMb(x)     ((x) / ONE_MB)
#define ByteToGb(x)     ((x) / ONE_GB)
#define ByteToTb(x)     ((x) / ONE_TB)

#define PAGE_SIZE_4K    (4 * ONE_KB)
#define PAGE_SIZE_2M    (2 * ONE_MB)
#define PAGE_SIZE_1G    (1 * ONE_GB)

#define PAGE_SIZE       PAGE_SIZE_4K        // default page size

#define PAGE_ALIGN(Va)      ((PVOID)((QWORD)(Va) & ~(PAGE_SIZE - 1)))
#define PHYPAGE_ALIGN(Va)   ((QWORD)((QWORD)(Va) & ~(PAGE_SIZE - 1)))

#define SMALL_PAGE_COUNT(sz)    (ROUND_UP((sz), PAGE_SIZE_4K) / PAGE_SIZE_4K)
#define LARGE_PAGE_COUNT(sz)    (ROUND_UP((sz), PAGE_SIZE_2M) / PAGE_SIZE_2M)
#define SUPER_PAGE_COUNT(sz)    (ROUND_UP((sz), PAGE_SIZE_1G) / PAGE_SIZE_1G)

//
// PTE flags
//
#define PTE_P           0x0001
#define PTE_RW          0x0002
#define PTE_US          0x0004
#define PTE_PWT         0x0008
#define PTE_PCD         0x0010
#define PTE_A           0x0020
#define PTE_D           0x0040
#define PTE_PAT         0x0080
#define PTE_XD          0x8000000000000000

//
// PDE flags
//
#define PDE_P           0x0001
#define PDE_RW          0x0002
#define PDE_US          0x0004
#define PDE_PWT         0x0008
#define PDE_PCD         0x0010
#define PDE_A           0x0020
#define PDE_D           0x0040   
#define PDE_PS          0x0080
#define PDE_XD          0x8000000000000000

//
// PDPE flags
//
#define PDPE_P          0x0001
#define PDPE_RW         0x0002
#define PDPE_US         0x0004
#define PDPE_PWT        0x0008
#define PDPE_PCD        0x0010
#define PDPE_A          0x0020
#define PDPE_PS         0x0080
#define PDPE_XD         0x8000000000000000

//
// PML4E flags
//
#define PML4E_P         0x0001
#define PML4E_RW        0x0002
#define PML4E_US        0x0004
#define PML4E_PWT       0x0008
#define PML4E_PCD       0x0010
#define PML4E_A         0x0020
#define PML4E_PS        0x0080
#define PML4E_XD        0x8000000000000000

#define PAGE_MASK       0xFFFFFFFFFFFFF000
#define PAGE_OFFSET     0xFFF
#define PHYS_PAGE_MASK  0x000FFFFFFFFFF000

#endif // !_MEMDEFS_H_
