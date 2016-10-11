#include "defs.h"
#include "screen.h"

static PSCREEN gVideoBuffer = (PSCREEN)(0x000B8000);
static VGA_COLOR gCurrentColor = vgaColorWhite;


void HelloBoot()
{
    int i, len;
	char boot[] = "Hello Boot! Greetings from C...";

	len = 0;
	while (boot[len] != 0)
	{
		len++;
	}

	for (i = 0; (i < len) && (i < MAX_OFFSET); i++)
	{
		gVideoBuffer[i].Color = 10;
		gVideoBuffer[i].Character = boot[i];
	}
}


VOID
ScrSetColor(
    VGA_COLOR Color
)
{
    gCurrentColor = Color;
}
