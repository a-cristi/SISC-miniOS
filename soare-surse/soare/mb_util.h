#ifndef _MB_UTIL_H_
#define _MB_UTIL_H_

#define MB_BOOT_DEV_DRIVE(dev)      ((dev) & 0xFF)
#define MB_BOOT_DEV_PART1(dev)      (((dev) >>  8) & 0xFF)
#define MB_BOOT_DEV_PART2(dev)      (((dev) >> 16) & 0xFF)
#define MB_BOOT_DEV_PART3(dev)      (((dev) >> 24) & 0xFF)

BOOLEAN
MbInterpretMultiBootInfo(
    _In_ PMULTIBOOT_INFO MultibootInfo
);

#endif // !_MB_UTIL_H_
