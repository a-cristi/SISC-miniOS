#ifndef _CRT_MEMORY_H_
#define _CRT_MEMORY_H_

//#pragma intrinsic(memcpy, memset, memcmp)

VOID* __cdecl
memcpy(
    __out_bcount_full_opt(Size) VOID *Dest,
    __in_bcount_opt(Size) const VOID *Source,
    __in SIZE_T Size
);

INT32 __cdecl
memcmp(
    __in_bcount_opt(Size) const VOID *Source1,
    __in_bcount_opt(Size) const VOID *Source2,
    __in SIZE_T Size
);

VOID* __cdecl
memset(
    __out_bcount_full_opt(Size) VOID *Dest,
    __in INT32 Value,
    __in SIZE_T Size\
);

VOID* __cdecl
memcpy_s(
    __out_bcount_full_opt(Size) VOID *Dest,
    __in SIZE_T SizeInBytes,
    __in_bcount_opt(Size) const VOID *Source,
    __in SIZE_T Size
);

VOID* __cdecl
memzero(
    __out_bcount_full_opt(Size) VOID *Dest,
    __in SIZE_T Size
);


#endif // _CRT_MEMORY_H_
