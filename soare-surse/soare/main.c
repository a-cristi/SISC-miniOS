#include "boot.h"
#include "screen.h"
#include "log.h"

extern void __DbgBochsBreak(void);

void EntryPoint(void)
{
    VgaInit(VGA_MEMORY_BUFFER, vgaColorWhite, vgaColorBlack);
    VgaPutString("Hello");
    Log("This is a simple string\n");
    LogWithInfo("This is a simple string with info\n");
    Log("This is a pointer followed by another string: %018p, %s\n", &EntryPoint, ":)");
    LogWithInfo("As above but with info: %018p, %s\n", &EntryPoint, ":)");
    __halt();
}
