#include "defs.h"
#include "screen.h"


static PSCREEN gVideo = (PSCREEN)(0x000B8000);

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
		gVideo[i].Color = 10;
		gVideo[i].Character = boot[i];
	}
}

