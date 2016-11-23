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

#endif // !_MEMDEFS_H_
