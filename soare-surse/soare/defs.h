#ifndef _DEFS_H_
#define _DEFS_H_

//
// generic version definitions for Win32 / WDK
//
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT_WINXP      0x0501
#define _WIN32_WINNT_VISTA      0x0600
#define _WIN32_WINNT_WIN7       0x0601
#define _WIN32_WINNT_WIN8       0x0602
#define _WIN32_WINNT            _WIN32_WINNT_WIN7
#endif

#if !defined(WINVER)
#define WINVER                  _WIN32_WINNT
#endif

// default NTDDI_VERSION
#if !defined(NTDDI_VERSION)
#define NTDDI_WINXPSP3          0x05010300
#define NTDDI_VISTASP2          0x06000200
#define NTDDI_WIN7SP1           0x06010100
#define NTDDI_WIN8              0x06020000
#define NTDDI_VERSION           NTDDI_WIN7SP1
#endif


//
// include SAL and VARARGS from Microsoft WDK
//
#include "shared\specstrings.h"
//#include "crt\varargs.h"

//
// skip certeain warnings
//
///#pragma warning(disable:4200)   // nonstandard extension, zero-sized array in struct/union
#pragma warning(disable:4201)   // nonstandard extension used : nameless struct/union
///#pragma warning(disable:4152)   // nonstandard extension, function/data pointer conversion in expression
///#pragma warning(disable:4214)   // nonstandard extension used : bit field types other than int
#pragma warning(disable:4127)   // conditional expression is constant


//
// default types
//
typedef unsigned __int8     BYTE, *PBYTE;
typedef unsigned __int16    WORD, *PWORD;
typedef unsigned __int32    DWORD, *PDWORD;
typedef unsigned __int64    QWORD, *PQWORD;
typedef signed __int8       INT8, *PINT8;
typedef signed __int16      INT16, *PINT16;
typedef signed __int32      INT32, *PINT32;
typedef signed __int64      INT64, PINT64;

#ifndef UINT8
#define UINT8               BYTE
#endif // !UINT8

#ifndef UINT16
#define UINT16              WORD
#endif // !UINT16

#ifndef UINT32
#define UINT32              DWORD
#endif // !UINT32

#ifndef UINT64
#define UINT64              QWORD
#endif // !UINT64

typedef char                CHAR, *PCHAR;
typedef unsigned char       UCHAR, *PUCHAR;
typedef unsigned __int16    WCHAR, *PWCHAR;

typedef unsigned __int64    SIZE_T, *PSIZE_T;

#ifndef size_t
#define size_t              SIZE_T
#endif // !size_t

typedef void                VOID, *PVOID;

typedef BYTE                BOOLEAN, *PBOOLEAN;

typedef INT32               NTSTATUS;

//
// frequently used definitions
//
#define UNREFERENCED_PARAMETER(P)           (P)
#define UNREFERENCED_LOCAL_VARIABLE(V)      (V)

#define ROUND_DOWN(val, align)     ((((val) % (align)) == 0) ? (val) : ((val) - ((val) % (align))))
#define ROUND_UP(val, align)       ((((val) % (align)) == 0) ? (val) : ((val) + ((align) - ((val) % (align)))))

#define MIN(x, y)           (((x) < (y)) ? (x) : (y))
#define MAX(x, y)           (((x) > (y)) ? (x) : (y))

#define TRUE                1
#define FALSE               0

#define NULL                ((PVOID)0)

/// TODO: implement this
#define assert(x)      

#define BIT(i)              (1ULL << (i))

#include <sal.h>

#endif // _DEFS_H_