#include "defs.h"
#include "memory.h"

VOID* __cdecl
memcpy(
    __out_bcount_full_opt(Size) VOID *Dest,
    __in_bcount_opt(Size) const VOID *Source,
    __in SIZE_T Size
)
{
    VOID *ret = Dest;

    assert(NULL != Dest);
    assert(NULL != Source);

    if ((NULL == Dest) || (NULL == Source))
    {
        return NULL;        // On release assert doesn't build
    }

    // copy from lower addresses to higher addresses
    while (Size--)
    {
        *(INT8 *)Dest = *(INT8 *)Source;
        Dest = (INT8 *)Dest + 1;
        Source = (INT8 *)Source + 1;
    }

    return(ret);
}


INT32 __cdecl
memcmp(
    __in_bcount_opt(Size) const VOID *Source1,
    __in_bcount_opt(Size) const VOID *Source2,
    __in SIZE_T Size
)
{
    assert(NULL != Source1);
    assert(NULL != Source2);
    assert(Size > 0);

    if ((NULL == Source1) || (NULL == Source2) || (Size <= 0))
    {
        return 0;           // There's no better return value, even if 0 might be confusing.
                            // We must return a value for release builds, because assert builds only for debug.
    }

    while (--Size && *(INT8 *)Source1 == *(INT8 *)Source2)
    {
        Source1 = (INT8 *)Source1 + 1;
        Source2 = (INT8 *)Source2 + 1;
    }

    return(*((UINT8 *)Source1) - *((UINT8 *)Source2));
}


VOID* __cdecl
memset(
    __out_bcount_full_opt(Size) VOID *Dest,
    __in INT32 Value,
    __in SIZE_T Size
)
{
    VOID *start = Dest;

    assert(NULL != Dest);

    if (NULL == Dest)
    {
        return NULL;
    }

    while (Size--)
    {
        *(INT8 *)Dest = (INT8)Value;
        Dest = (INT8 *)Dest + 1;
    }

    return(start);
}

VOID* __cdecl
memcpy_s(
    __out_bcount_full_opt(Size) VOID *Dest,
    __in SIZE_T SizeInBytes,
    __in_bcount_opt(Size) const VOID *Source,
    __in SIZE_T Size)
{
    if (0 == Size)
    {
        return NULL;
    }

    if ((NULL == Source) || (SizeInBytes < Size))
    {
        memzero(Dest, Size);
        return NULL;
    }

    memcpy(Dest, Source, Size);

    return Dest;
}


VOID* __cdecl
memzero(
    __out_bcount_full_opt(Size) VOID *Dest,
    __in SIZE_T Size
)
{
    return memset(Dest, 0, Size);
}
