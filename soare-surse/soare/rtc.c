#include "defs.h"
#include "ntstatus.h"
#include "rtc.h"
#include "log.h"

//
// Registers
//
#define RTC_REG_SECONDS         0x00
#define RTC_REG_MINUTES         0x02
#define RTC_REG_HOURS           0x04
#define RTC_REG_WEEKDAY         0x06
#define RTC_REG_DAY_OF_MONTH    0x07
#define RTC_REG_MONTH           0x08
#define RTC_REG_YEAR            0x09
#define RTC_REG_CENTURY         0x32        // maybe, see http://wiki.osdev.org/CMOS "Century Register"
#define RTC_REG_STATUS_A        0x0A
#define RTC_REG_STATUS_B        0x0B

//
// Status A bits
//
#define RTC_STATUS_A_UIP        (1 << 7)    // update in progress

//
// Status B bits
//
#define RTC_STATUS_B_24H        0x02        // enables 24 hour format
#define RTC_STATUS_B_BINARY     0x04        // enabled binary mode

#define RTC_HOUR_12H_PM         0x80

//
// CMOS
//
#define CMOS_ADDRESS            0x70
#define CMOS_DATA               0x71

#define CMOS_NMI_BIT            0x80


#define CURRENT_CENTURY         20

static
__forceinline
BYTE
_RtcReadRegister(
    _In_ BYTE Register
)
{
    __outbyte(CMOS_ADDRESS, Register | CMOS_NMI_BIT);
    return __inbyte(CMOS_DATA);
}


static 
__forceinline
VOID
_RtcWaitForUpdate(
    VOID
)
{
    // It is not enough to wait until the update in progress bit is cleared, 
    // we should wait for it to be set and then cleared, so we will have one second at disposition to read the RTC values
    while (0 == (_RtcReadRegister(RTC_REG_STATUS_A) & RTC_STATUS_A_UIP));   // wait until an update starts
    while (0 != (_RtcReadRegister(RTC_REG_STATUS_A) & RTC_STATUS_A_UIP));   // wait until the update is finished
}


NTSTATUS
RtcGetTimeAndDate(
    _Inout_ PRTC_DATE_TIME DateTime
)
{
    BYTE stsReg;

    if (!DateTime)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    _RtcWaitForUpdate();
    DateTime->Seconds = _RtcReadRegister(RTC_REG_SECONDS);
    DateTime->Minutes = _RtcReadRegister(RTC_REG_MINUTES);
    DateTime->Hours = _RtcReadRegister(RTC_REG_HOURS);
    DateTime->DayOfMonth = _RtcReadRegister(RTC_REG_DAY_OF_MONTH);
    DateTime->Month = _RtcReadRegister(RTC_REG_MONTH);
    DateTime->Year = _RtcReadRegister(RTC_REG_YEAR);

    stsReg = _RtcReadRegister(RTC_REG_STATUS_B);

    // BCD?
    if (0 == (stsReg & RTC_STATUS_B_BINARY))
    {
        DateTime->Seconds = RtcBcdToBinary(DateTime->Seconds);
        DateTime->Minutes = RtcBcdToBinary(DateTime->Minutes);
        DateTime->Hours = RtcBcdToBinary(DateTime->Hours);
        DateTime->DayOfMonth = RtcBcdToBinary(DateTime->DayOfMonth);
        DateTime->Month = RtcBcdToBinary(DateTime->Month);
        DateTime->Year = RtcBcdToBinary(DateTime->Year);
    }

    // 12 hour format?
    if ((0 == (stsReg & RTC_STATUS_B_24H)) && (0 != (DateTime->Hours & RTC_HOUR_12H_PM)))
    {
        DateTime->Hours = ((DateTime->Hours & 0x7F) + 12) % 24;
    }

    // TODO: some year/century checking?
    DateTime->Year += (CURRENT_CENTURY * 100);

    return STATUS_SUCCESS;
}