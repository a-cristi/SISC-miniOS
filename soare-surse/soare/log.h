#ifndef _LOG_H_
#define _LOG_H_

#define KLOG_MAX_CHARS  512

#define LOG_MEDIUM_VGA      (1 << 0)
#define LOG_MEDIUM_SERIAL   (1 << 1)
#define LOG_MEDIUM_ALL      (LOG_MEDIUM_VGA | LOG_MEDIUM_SERIAL)

#define LOG_MEDIUM_DEFAULT  LOG_MEDIUM_SERIAL

INT32
KLogWithInfo(
    _In_ PCHAR File,
    _In_ DWORD Line,
    _In_ PCHAR Format,
    ...
);

INT32
KLog(
    _In_ DWORD Flags,
    _In_ PCHAR Format,
    ...
);

INT32
KPrint(
    _In_ PCHAR Format,
    ...
);

#define FNAME(MaxChars) ((sizeof(__FILE__) - 1 <= (MaxChars)) ? (__FILE__) : ((__FILE__) + sizeof(__FILE__) - 1 - (MaxChars)))

#define LogWithInfo(fmt, ...)       KLogWithInfo(FNAME(16), __LINE__, fmt, __VA_ARGS__)
#define NLog(fmt, ...)              KLog(LOG_MEDIUM_DEFAULT, fmt, __VA_ARGS__)

#define Log(fmt, ...)               KLog(LOG_MEDIUM_ALL, fmt, __VA_ARGS__)

#define Display(fmt, ...)           KLog(LOG_MEDIUM_VGA, fmt, __VA_ARGS__)

#endif // !_LOG_H_