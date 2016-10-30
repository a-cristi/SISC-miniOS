#ifndef _LOG_H_
#define _LOG_H_

#define KLOG_MAX_CHARS  512

INT32
KLogWithInfo(
    _In_ PCHAR File,
    _In_ DWORD Line,
    _In_ PCHAR Format,
    ...
);

INT32
KLog(
    _In_ PCHAR Format,
    ...
);

#define FNAME(MaxChars) ((sizeof(__FILE__) - 1 <= (MaxChars)) ? (__FILE__) : ((__FILE__) + sizeof(__FILE__) - 1 - (MaxChars)))

#define LogWithInfo(fmt, ...)       KLogWithInfo(FNAME(16), __LINE__, fmt, __VA_ARGS__)
#define Log(fmt, ...)               KLog(fmt, __VA_ARGS__)

#endif // !_LOG_H_