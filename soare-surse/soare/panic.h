#ifndef _PANIC_H_
#define _PANIC_H_

__declspec(noreturn)
VOID
DbgPanicWithInfo(
    _In_opt_ PCHAR File,
    _In_ DWORD Line,
    _In_ PCHAR Reason
);

#define PANIC(msg)              DbgPanicWithInfo(__FILE__, __LINE__, (msg))

#endif // !_PANIC_H_
