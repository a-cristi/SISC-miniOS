#include "defs.h"
#include "varargs.h"
#include "string.h"
#include "log.h"
#include "screen.h"
#include "serial.h"

INT32
KLog(
    _In_ DWORD Flags,
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

    if (0 != (LOG_MEDIUM_SERIAL & Flags))
    {
        SerialPutString(buf, len + 1);
    }
    if (0 != (LOG_MEDIUM_VGA & Flags))
    {
        VgaPutString(buf);
    }

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

    len = KLog(LOG_MEDIUM_DEFAULT, "%s:%d\t%s", File, Line, buf);

    return len;
}
