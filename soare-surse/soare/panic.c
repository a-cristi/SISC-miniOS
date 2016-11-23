#include "defs.h"
#include "log.h"
#include "screen.h"
#include "debugger.h"
#include "panic.h"

__declspec(noreturn)
VOID
DbgPanicWithInfo(
    _In_opt_ PCHAR File,
    _In_ DWORD Line,
    _In_ PCHAR Reason
)
{
    VgaClearScreenToColor(vgaColorBlue, vgaColorBlue);
    VgaSetForeground(vgaColorRed);

    Log("FATAL ERROR: %s\n", Reason ? Reason : "UNKNOWN");

    if (File)
    {
        Log("System stop requested from: %s:%d\n", File, Line);
    }

    DbgBreakWithInfo("If a debugger is present, you can inspect the system state.\n");
    _disable();
    __halt();
    while (TRUE);
}
