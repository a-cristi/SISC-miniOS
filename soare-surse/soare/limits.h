#ifndef _CRT_LIMITS_H_
#define _CRT_LIMITS_H_

#define MB_LEN_MAX    5             // max. # bytes in multibyte char
#define SHRT_MIN    (-32768)        // minimum (signed) short value
#define SHRT_MAX      32767         // maximum (signed) short value
#define USHRT_MAX     0xffff        // maximum unsigned short value
#define INT_MIN     (-2147483647 - 1) // minimum (signed) int value
#define INT_MAX       2147483647    // maximum (signed) int value
#define UINT_MAX      0xffffffff    // maximum unsigned int value
#define LONG_MIN    (-2147483647L - 1) // minimum (signed) long value
#define LONG_MAX      2147483647L   // maximum (signed) long value
#define ULONG_MAX     0xffffffffUL  // maximum unsigned long value
#define LLONG_MAX     9223372036854775807i64       // maximum signed long long int value
#define LLONG_MIN   (-9223372036854775807i64 - 1)  // minimum signed long long int value
#define ULLONG_MAX    0xffffffffffffffffui64       // maximum unsigned long long int value

#define _I8_MIN     (-127i8 - 1)    // minimum signed 8 bit value
#define _I8_MAX       127i8         // maximum signed 8 bit value
#define _UI8_MAX      0xffui8       // maximum unsigned 8 bit value

#define _I16_MIN    (-32767i16 - 1) // minimum signed 16 bit value
#define _I16_MAX      32767i16      // maximum signed 16 bit value
#define _UI16_MAX     0xffffui16    // maximum unsigned 16 bit value

#define _I32_MIN    (-2147483647i32 - 1) // minimum signed 32 bit value
#define _I32_MAX      2147483647i32 // maximum signed 32 bit value
#define _UI32_MAX     0xffffffffui32 // maximum unsigned 32 bit value

#endif // !_CRT_LIMITS_H_
