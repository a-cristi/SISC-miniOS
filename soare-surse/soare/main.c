#include "boot.h"
#include "screen.h"
#include "log.h"
#include "pic.h"

extern void __DbgBochsBreak(void);

void EntryPoint(void)
{
    VgaInit(VGA_MEMORY_BUFFER, vgaColorWhite, vgaColorBlack);
    Log("> Initializing PIC... ");
    PicInitialize();
    Log("Done!\n");
    __DbgBochsBreak();
    __halt();
}
