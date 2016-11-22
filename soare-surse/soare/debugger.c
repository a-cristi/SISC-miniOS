#include "defs.h"
#include "log.h"

extern VOID __DbgBochsBreak(VOID);

VOID
DbgEnterDebuggerWithInfo(
    _In_opt_ PCHAR File,
    _In_ DWORD Line,
    _In_opt_ PCHAR Reason
)
{
    if (File)
    {
        Log("Debug break requested from %s:%d with message: %s\n", File, Line, Reason ? Reason : "");
    }
    else
    { 
        Log("Debug break requested with message: %s\n", Reason ? Reason : "");
    }
    
    __DbgBochsBreak();
}

VOID
DbgBreak(
    VOID
)
{
    __DbgBochsBreak();
}
