#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

VOID
DbgEnterDebuggerWithInfo(
    _In_opt_ PCHAR File,
    _In_ DWORD Line,
    _In_opt_ PCHAR Reason
);

VOID
DbgBreak(
    VOID
);

#define DbgBreakWithInfo(msg)           DbgEnterDebuggerWithInfo(__FILE__, __LINE__, (msg))

#endif // !_DEBUGGER_H_
