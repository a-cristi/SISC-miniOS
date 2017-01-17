#include "defs.h"
#include "varargs.h"
#include "string.h"
#include "memory.h"

#pragma function(strlen)
//
// strlen
//
size_t __cdecl
strlen(
    __in_z const char *str
)
{
    const char *eos = str;

    assert(NULL != str);

    if (NULL == str)
    {
        return 0;
    }

    while (*eos++);

    return(eos - str - 1);
}


#pragma function(strcpy)
//
// strcpy
//
char * __cdecl
strcpy(
    __out_z char * dst,
    __in_z const char * src
)
{
    char * cp = dst;

#pragma warning(suppress:4127)
    while (TRUE)
    {
        *cp = *src;
        if (0 == *cp)
            break;
        cp++;
        src++;
    }

    return(dst);
}

#pragma function(strcat)
//
// strcat
//
char * __cdecl
strcat(
    __out_z char * dst,
    __in_z const char * src
)
{
    char * cp = dst;

    while (*cp)
        cp++;                   // find end of dst

#pragma warning(suppress:4127)
    while (TRUE)
    {
        *cp = *src;
        if (0 == *cp)
            break;
        cp++;
        src++;
    }

    return(dst);                  // return dst
}


#pragma function(strcmp)
//
// strcmp
//
int __cdecl
strcmp(
    __in_z const char *str1,
    __in_z const char *str2
)
{
    int ret = 0;

    assert(NULL != str1);
    assert(NULL != str2);

    if (NULL == str1 || NULL == str2)
    {
        return 0;           // Might be confusing...
    }

    ///    while( ! (ret = *(unsigned char *)str1 - *(unsigned char *)str2) && *str2)
    ///            ++str1, ++str2;
    do
    {
        ret = *(unsigned char *)(str1++) - *(unsigned char *)(str2++);
    } while (!ret && *str2);

    if (ret < 0)
        ret = -1;
    else if (ret > 0)
        ret = 1;

    return(ret);
}


//#endif


//
// wstrlen
//
size_t __cdecl
wstrlen(
    __in_z const PWCHAR str
)
{
    size_t i;

    assert(NULL != str);

    if (NULL == str)
    {
        return 0;
    }

    for (i = 0; str[i] != 0; i++);

    return i;
}


//
// strstr
//
char * __cdecl
strstr(
    __in_z const char *str1,
    __in_z const char *str2
)
{
    char *cp = (char *)str1;
    char *s1, *s2;

    assert(NULL != str1);
    assert(NULL != str2);

    if (NULL == str1 || NULL == str2)
    {
        return NULL;
    }

    if (!*str2)
        return((char *)str1);

    while (*cp)
    {
        s1 = cp;
        s2 = (char *)str2;

        while (*s1 && *s2 && !(*s1 - *s2))
            s1++, s2++;

        if (!*s2)
            return(cp);

        cp++;
    }

    return(NULL);
}


//
// strchr
//
char * __cdecl
strchr(
    __in_z const char *str,
    __in char c
)
{
    assert(NULL != str);

    if (NULL == str)
    {
        return NULL;
    }

    while (*str && *str != (char)c)
        str++;

    if (*str == (char)c)
        return((char *)str);

    return(NULL);
}


//
// strrchr
//
char * __cdecl
strrchr(
    __in_z const char *str,
    __in char c
)
{
    char *start = (char *)str;

    assert(NULL != str);
    if (NULL == str)
    {
        return NULL;
    }

    while (*str++)                          /* find end of string */
        ;
    /* search towards front */
    while (--str != start && *str != (char)c)
        ;

    if (*str == (char)c)                    /* char found ? */
        return((char *)str);

    return(NULL);
}


//
// stricmp
//
int __cdecl
stricmp(
    __in_z const char *str1,
    __in_z const char *str2
)
{
    int f, l;

    do
    {
        if (((f = (unsigned char)(*(str1++))) >= 'A') && (f <= 'Z'))
            f -= 'A' - 'a';
        if (((l = (unsigned char)(*(str2++))) >= 'A') && (l <= 'Z'))
            l -= 'A' - 'a';
    } while (f && (f == l));

    return(f - l);
}


//
// strncmp
//
int __cdecl
strncmp(
    __in_z /*const*/ char *first,
    __in_z const char *last,
    __in size_t count
)
{
    size_t x = 0;

    if (!count)
    {
        return 0;
    }

    /*
    * This explicit guard needed to deal correctly with boundary
    * cases: strings shorter than 4 bytes and strings longer than
    * UINT_MAX-4 bytes .
    */
    if (count >= 4)
    {
        /* unroll by four */
        for (; x < count - 4; x += 4)
        {
            first += 4;
            last += 4;

            if (*(first - 4) == 0 || *(first - 4) != *(last - 4))
            {
                return(*(unsigned char *)(first - 4) - *(unsigned char *)(last - 4));
            }

            if (*(first - 3) == 0 || *(first - 3) != *(last - 3))
            {
                return(*(unsigned char *)(first - 3) - *(unsigned char *)(last - 3));
            }

            if (*(first - 2) == 0 || *(first - 2) != *(last - 2))
            {
                return(*(unsigned char *)(first - 2) - *(unsigned char *)(last - 2));
            }

            if (*(first - 1) == 0 || *(first - 1) != *(last - 1))
            {
                return(*(unsigned char *)(first - 1) - *(unsigned char *)(last - 1));
            }
        }
    }

    /* residual loop */
    for (; x < count; x++)
    {
        if (*first == 0 || *first != *last)
        {
            return(*(unsigned char *)first - *(unsigned char *)last);
        }
        first += 1;
        last += 1;
    }

    return 0;
}


//
// strnicmp
//
int __cdecl
strnicmp(
    __in_z const char *str1,
    __in_z const char *str2,
    __in size_t count
)
{
    int f = 0;        // first, last
    int l = 0;

    if (0 == count)
    {
        return 0;
    }

    do
    {
        if (((f = (unsigned char)(*(str1++))) >= 'A') && (f <= 'Z'))
            f -= 'A' - 'a';

        if (((l = (unsigned char)(*(str2++))) >= 'A') && (l <= 'Z'))
            l -= 'A' - 'a';
    } while (--count && f && (f == l));

    return (f - l);
}


//
// strend
//
char * __cdecl
strend(
    __in_z const char *str,
    __in size_t count
)
{
    const char *eos = str;
    size_t len;

    assert(NULL != str);

    if (NULL == str)
    {
        return NULL;
    }

    while (*eos++);

    len = (eos - str - 1);

    if (count >= len)
    {
        return (char*)str;
    }
    else
    {
        return (char*)&(str[len - count]);
    }
}


//
// PRINTF family
//

//
// snprintf
//
int __cdecl
snprintf(
    __out_z char *buffer,
    __in size_t count,
    __in_z const char *format,
    ...
)
{
    va_list arglist;
    int retval;

    va_start(arglist, format);

    retval = vsnprintf(buffer, count, format, arglist);

    va_end(arglist);

    return retval;
}


size_t __cdecl
strlen_s(
    __in_z const char *str,
    __in size_t size)
{
    size_t len = 0;

    assert(NULL != str);

    if (NULL == str)
    {
        return 0;
    }

    for (len = 0; (len < size) && (str[len]); len++);

    if (str[len] == 0)
    {
        return len;
    }
    else
    {
        return (size_t)-1;
    }
}


size_t __cdecl
wstrlen_s(
    __in_z const PWCHAR str,
    __in size_t size)
{
    size_t len = 0;

    assert(NULL != str);

    if (NULL == str)
    {
        return 0;
    }

    for (len = 0; (len < size) && (str[len]); len++);

    if (str[len] == 0)
    {
        return len;
    }
    else
    {
        return (size_t)-1;
    }
}


char * __cdecl
strcpy_s(
    __out_z char *dst,
    __in size_t dst_size,
    __in_z const char *src)
{
    char * cp = dst;
    size_t s = 0;

    while (s++ < dst_size)
    {
        *cp = *src;
        if (0 == *cp)
            break;
        cp++;
        src++;
    }

    if (s == dst_size && dst[s] != 0)
    {
        memzero(dst, dst_size);
        return NULL;
    }

    return(dst);
}

char * __cdecl
strcat_s(
    __out_z char *dst,
    __in size_t dst_size,
    __in_z const char *src)
{
    char *p;
    size_t available;

    assert(NULL != dst);
    assert(NULL != src);

    p = dst;
    available = dst_size;
    while (available > 0 && *p != 0)
    {
        p++;
        available--;
    }

    if (available == 0)
    {
        memzero(dst, dst_size);
        return NULL;
    }

    while ((*p++ = *src++) != 0 && --available > 0);

    if (available == 0)
    {
        memzero(dst, dst_size);
        return NULL;
    }

    return dst;
}


char * __cdecl
strstr_s(
    __in_z const char *str,
    __in size_t str_size,
    __in_z const char *strSearch)
{
    char *cp = (char *)str;
    char *s1, *s2;
    size_t i = 0, j = 0;

    assert(NULL != str);
    assert(NULL != strSearch);

    if (NULL == str || NULL == strSearch)
    {
        return NULL;
    }

    if (!*strSearch)
        return((char *)str);

    while (*cp && i < str_size)
    {
        s1 = cp;
        s2 = (char *)strSearch;
        j = i;

        while (*s1 && *s2 && j < str_size && !(*s1 - *s2))
            s1++, s2++, j++;

        if (!*s2)
            return(cp);

        cp++;
        i++;
    }

    return(NULL);
}

char * __cdecl
strchr_s(
    __in_z const char *str,
    __in size_t str_size,
    __in char c)
{
    size_t i = 0;

    assert(NULL != str);
    if (NULL == str)
    {
        return NULL;
    }

    while (*str && i < str_size && *str != (char)c)
        str++;

    if (*str == (char)c)
        return((char *)str);

    return(NULL);
}

char * __cdecl
strrchr_s(
    __in_z const char *str,
    __in size_t str_size,
    __in char c)
{
    char *start = (char *)str;
    size_t i = 0;

    assert(NULL != str);
    if (NULL == str)
    {
        return NULL;
    }

    while (*str++ && i++ < str_size);       /* find end of string */

    if (i == str_size && start[i] != 0)
    {
        return NULL;
    }

    /* search towards front */
    while (--str != start && *str != (char)c)
        ;

    if (*str == (char)c)                    /* char found ? */
        return((char *)str);

    return(NULL);
}


char * __cdecl
strend_s(
    __in_z const char *str,
    __in size_t str_size,
    __in size_t count)
{
    const char *eos = str;
    size_t len;
    size_t i = 0;

    assert(NULL != str);
    if (NULL == str)
    {
        return NULL;
    }

    while (*eos++ && i++ < str_size);

    if (i == str_size && str[i] != 0)
    {
        return NULL;
    }

    len = (eos - str - 1);

    if (count >= len)
    {
        return (char*)str;
    }
    else
    {
        return (char*)&(str[len - count]);
    }
}

int __cdecl
wstrcmp(
    __in_z const PWCHAR lhs,
    __in_z const PWCHAR rhs,
    __in SIZE_T len
)
{
    SIZE_T i;

    if ((NULL == lhs) || (NULL == rhs))
    {
        return 0;
    }

    for (i = 0; i < len; i++)
    {
        char c1, c2;

        c1 = (char)lhs[i];
        c2 = (char)rhs[i];

        if (c1 >= 'A' && c1 <= 'Z')
        {
            c1 |= 0x20;
        }

        if (c2 >= 'A' && c2 <= 'Z')
        {
            c2 |= 0x20;
        }

        if (c1 < c2)
            return -1;
        else if (c1 > c2)
            return 1;
    }

    return 0;
}
