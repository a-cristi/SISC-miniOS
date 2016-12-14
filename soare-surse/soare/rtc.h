#ifndef _RTC_H_
#define _RTC_H_

typedef struct _RTC_DATE_TIME
{
    BYTE        Seconds;
    BYTE        Minutes;
    BYTE        Hours;
    BYTE        DayOfMonth;
    BYTE        Month;
    WORD        Year;
} RTC_DATE_TIME, *PRTC_DATE_TIME;


#define RtcBcdToBinary(Value)       ((((Value) & 0xF0) >> 1) + (((Value) & 0xF0) >> 3) + ((Value) & 0xf))


NTSTATUS
RtcGetTimeAndDate(
    _Inout_ PRTC_DATE_TIME DateTime
);

#endif // !_RTC_H_
