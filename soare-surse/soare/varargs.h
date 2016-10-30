//
//  NOTE: based on Microsoft Visual C CRT source
//
#ifndef _CRT_VARARGS_H_
#define _CRT_VARARGS_H_

typedef char *  va_list;

#ifdef  __cplusplus
#define _ADDRESSOF(v)           ( &reinterpret_cast<const char &>(v) )
#else
#define _ADDRESSOF(v)           ( &(v) )
#endif

#if defined(AMD64) || defined(WIN64)

extern void __cdecl __va_start(__out va_list *, ...);       // is this exported by VC compiler?

#define _crt_va_start(ap, x)    ( __va_start(&ap, x) )
#define _crt_va_arg(ap, t)      ( ( sizeof(t) > sizeof(__int64) || ( sizeof(t) & (sizeof(t) - 1) ) != 0 ) \
                                    ? **(t **)( ( ap += sizeof(__int64) ) - sizeof(__int64) ) \
                                    :  *(t  *)( ( ap += sizeof(__int64) ) - sizeof(__int64) ) )
#define _crt_va_end(ap)         ( ap = (va_list)0 )

#else

// a guess at the proper definitions for other platforms

#define _INTSIZEOF(n)           ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define _crt_va_start(ap,v)     ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define _crt_va_arg(ap,t)       ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define _crt_va_end(ap)         ( ap = (va_list)0 )

#endif

#define va_start _crt_va_start
#define va_arg _crt_va_arg
#define va_end _crt_va_end

#endif // _CRT_VARARGS_H_

