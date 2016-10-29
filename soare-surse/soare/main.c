#include "boot.h"
#include "screen.h"

extern void __DbgBochsBreak(void);

void EntryPoint(void)
{
    VgaInit(VGA_MEMORY_BUFFER, vgaColorWhite, vgaColorBlack);
    VgaPutString("Hello");
    __halt();
}
