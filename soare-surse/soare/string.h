//
//  NOTE: based on Microsoft Visual C CRT source
//
#ifndef _CRT_STRING_H_
#define _CRT_STRING_H_

//
// ANSI character macros
//
#define tolower(c)      ( (((c) >= 'A') && ((c) <= 'Z')) ? ((c) - 'A' + 'a') : (c) )

#define toupper(c)      ( (((c) >= 'a') && ((c) <= 'z')) ? ((c) - 'a' + 'A') : (c) )

#define isalpha(c)      ( ( (((c) >= 'A') && ((c) <= 'Z')) || \
                            (((c) >= 'a') && ((c) <= 'z')) ) ? 1 : 0 )

#define isdigit(c)      ( (((c) >= '0') && ((c) <= '9')) ? 1 : 0 )

#define isxdigit(c)     ( ( (((c) >= 'A') && ((c) <= 'F')) || \
                            (((c) >= 'a') && ((c) <= 'f')) || \
                            (((c) >= '0') && ((c) <= '9')) ) ? 1 : 0 )

//
// ANSI string functions
//
size_t __cdecl
strlen(
    __in_z const char *str);

char * __cdecl
strcpy(
    __out_z char *dst,
    __in_z const char *src);

char * __cdecl
strcat(
    __out_z char *dst,
    __in_z const char *src);

int __cdecl
strcmp(
    __in_z const char *str1,
    __in_z const char *str2);

#pragma intrinsic(strlen, strcpy, strcat, strcmp)

char * __cdecl
strstr(
    __in_z const char *str1,
    __in_z const char *str2);

char * __cdecl
strchr(
    __in_z const char *str,
    __in char c);

char * __cdecl
strrchr(
    __in_z const char *str,
    __in char c);

int __cdecl
stricmp(
    __in_z const char *str1,
    __in_z const char *str2);

int __cdecl
strncmp(
    __in_z /*const*/ char *str1,
    __in_z const char *str2,
    __in size_t count);

int __cdecl
strnicmp(
    __in_z const char *str1,
    __in_z const char *str2,
    __in size_t count);

char * __cdecl
strend(
    __in_z const char *str,
    __in size_t count);


//
// ANSI SAFE string functions
//

// Returns length, or -1 if string doesn't end within given size (size includes null terminator)
size_t __cdecl
strlen_s(
    __in_z const char *str,
    __in size_t size);

char * __cdecl
strcpy_s(
    __out_z char *dst,
    __in size_t dst_size,
    __in_z const char *src);

char * __cdecl
strcat_s(
    __out_z char *dst,
    __in size_t dst_size,
    __in_z const char *src);

char * __cdecl
strstr_s(
    __in_z const char *str,
    __in size_t str_size,
    __in_z const char *strSearch);

char * __cdecl
strchr_s(
    __in_z const char *str,
    __in size_t str_size,
    __in char c);

char * __cdecl
strrchr_s(
    __in_z const char *str,
    __in size_t str_size,
    __in char c);

char * __cdecl
strend_s(
    __in_z const char *str,
    __in size_t str_size,
    __in size_t count);


//
// PRINTF family prototypes
//
int __cdecl
rpl_vsnprintf(
    __out_z char *buffer,
    __in size_t count,
    __in_z const char *format,
    __in va_list ap);

#define vsnprintf rpl_vsnprintf

int __cdecl
snprintf(
    __out_z char *buffer,
    __in size_t count,
    __in_z const char *format,
    ...);

//
// wide strings
//

//
// wstrlen
//
size_t __cdecl
wstrlen(
    __in_z const PWCHAR str
);

size_t __cdecl
wstrlen_s(
    __in_z const PWCHAR str,
    __in size_t size);

int __cdecl
wstrcmp(
    __in_z const PWCHAR lhs,
    __in_z const PWCHAR rhs,
    __in SIZE_T len
);

#endif // _CRT_STRING_H_
