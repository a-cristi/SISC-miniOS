#ifndef _BOOT_H_
#define _BOOT_H_

#include "defs.h"
#include <intrin.h>

//
// MULTIBOOT 0.6.96 specifics
//
#define MULTIBOOT_HEADER_MAGIC 0x1BADB002

#pragma pack(push)
#pragma pack(1)

typedef struct _MULTIBOOT_HEADER
{
	DWORD           Magic;
	DWORD           Flags;
	DWORD           Checksum;
	DWORD           HeaderAddr;
	DWORD           LoadAddr;
	DWORD           LoadEndAddr;
	DWORD           BssEndAddr;
	DWORD           EntryAddr;
	DWORD           ModType;
	DWORD           Width;
	DWORD           Height;
	DWORD           Depth;
} MULTIBOOT_HEADER, *PMULTIBOOT_HEADER;

// MultiBoot Information structure
typedef struct _MULTIBOOT_INFO
{
	DWORD   Flags;
	union {
		QWORD       MemQuad;
		struct {
			DWORD   MemLower;
			DWORD   MemUpper;
		};
	};
	DWORD   BootDevice;
	DWORD   CmdLine;
	DWORD   ModsCount;
	DWORD   ModsAddr;
	BYTE    Reserved[16];

	DWORD   MmapLength;
	DWORD   MmapAddr;
	DWORD   DriversLength;
	DWORD   DriversAddr;

	DWORD   ConfigTable;

	DWORD   BootLoaderName;
	DWORD   ApmTable;

	DWORD   VbeControlInfo;
	DWORD   VbeModeInfo;
	DWORD   VbeMode;
	DWORD   VbeInterfaceSeg;
	DWORD   VbeInterfaceOff;
	DWORD   VbeInterfaceLen;
} MULTIBOOT_INFO, *PMULTIBOOT_INFO;

#pragma pack(pop)

/// ...

//
// exported functions from __init.asm
//
void __cli(void);
void __sti(void);
void __magic(void);         // MAGIC breakpoint into BOCHS (XCHG BX,BX)

#endif // _BOOT_H_