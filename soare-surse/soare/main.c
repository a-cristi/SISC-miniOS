#include "boot.h"
#include "screen.h"

void EntryPoint(void)
{
    VgaInit(VGA_MEMORY_BUFFER, vgaColorWhite, vgaColorBlack);
    __halt();
}
