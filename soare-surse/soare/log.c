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
        SerialPutString(buf, len);
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
    const DWORD tscDiv = 1000000;
    va_list ap;
    INT32 len;
    CHAR buf[KLOG_MAX_CHARS] = { 0 };

    va_start(ap, Format);
    len = rpl_vsnprintf(buf, KLOG_MAX_CHARS - 1, Format, ap);
    va_end(ap);

    len = KLog(LOG_MEDIUM_DEFAULT, "[%d.%06d] %s:%d\t%s", 
        __rdtsc() / tscDiv, __rdtsc() % tscDiv, File, Line, buf);

    return len;
}


INT32
KLogUpdateHeader(
    _In_ BOOLEAN RightToLeft,
    _In_ PCHAR Format,
    ...
)
{
    va_list ap;
    INT32 len;
    CHAR buf[VGA_HEADER_MAX_SIZE + 1] = { 0 };

    va_start(ap, Format);
    len = rpl_vsnprintf(buf, VGA_HEADER_MAX_SIZE, Format, ap);
    va_end(ap);

    VgaUpdateHeader(RightToLeft ? -1 * (WORD)len : 0, vgaColorLightBlue, vgaColorBlack, buf);

    return len;
}
