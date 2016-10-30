#include "defs.h"
#include "varargs.h"
#include "string.h"
#include "log.h"
#include "screen.h"

INT32
KLog(
    _In_ PCHAR Format,
    ...
)
{
    va_list ap;
    INT32 len;
    CHAR buf[KLOG_MAX_CHARS] = { 0 };

    va_start(ap, Format);
    len = rpl_vsnprintf(buf, KLOG_MAX_CHARS - 1, Format, ap);
    va_end(ap);

    VgaPutString(buf);

    return len;
}


INT32
KLogWithInfo(
    _In_ PCHAR File,
    _In_ DWORD Line,
    _In_ PCHAR Format,
    ...
)
{
    va_list ap;
    INT32 len;
    CHAR buf[KLOG_MAX_CHARS] = { 0 };

    va_start(ap, Format);
    len = rpl_vsnprintf(buf, KLOG_MAX_CHARS - 1, Format, ap);
    va_end(ap);

    len = KLog("%s:%d\t%s", File, Line, buf);

    return len;
}