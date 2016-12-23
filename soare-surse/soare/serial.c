#include "defs.h"
#include "serial.h"

#define COM1        0x3f8
#define COM2        0x2f8
#define COM3        0x3e8
#define COM4        0x2e8

#define COM_DEFAULT COM1

// 
// Configuration
//
//  IO Port Offset      Setting of DLAB     Register mapped to this port
//  +0                  0                   Data register; read from / write to when communicating
//  +1                  0                   Interrupt Enable Register
//  +0                  1                   With DLAB set to 1, this is the least significant byte of the divisor value for setting the baud rate.
//  +1                  1                   With DLAB set to 1, this is the most significant byte of the divisor value.
//  +2                  -                   Interrupt Identification and FIFO control registers
//  +3                  -                   Line Control Register. The most significant bit of this register is the DLAB.
//  +4                  -                   Modem Control Register.
//  +5                  -                   Line Status Register.
//  +6                  -                   Modem Status Register.
//  +7                  -                   Scratch Register

VOID
SerComInit(
    VOID
)
{
    // disable interrupts
    __outbyte(COM_DEFAULT + 1, 0x00);

    // enable DLAB
    __outbyte(COM_DEFAULT + 3, 0x80);

    // divisor low byte (baud rate 115200)
    __outbyte(COM_DEFAULT + 0, 0x01);

    // divisor hight byte
    __outbyte(COM_DEFAULT + 1, 0x00);

    // 8n1 (8 bits, no parity, 1 stop bit)
    __outbyte(COM_DEFAULT + 3, 0x03);

    // enable FIFO
    __outbyte(COM_DEFAULT + 2, 0xC7);
}


static
__forceinline
BOOLEAN
_SerRcvd(
    _In_ WORD Com
)
{
    return (0 != (__inbyte(Com + 5) & 1));
}


static
__forceinline
CHAR
_SerReadPort(
    _In_ WORD Com
)
{
    while (!_SerRcvd(Com));

    return __inbyte(Com);
}


CHAR
SerReadData(
    VOID
)
{
    return _SerReadPort(COM_DEFAULT);
}


static
__forceinline
BOOLEAN
_SerIsTransmitEmpty(
    _In_ WORD Com
)
{
    return (0 != (__inbyte(Com + 5) & 0x20));
}


static
__forceinline
VOID
_SerWritePort(
    _In_ WORD Com,
    _In_ CHAR Ch
)
{
    while (!_SerIsTransmitEmpty(Com));

    __outbyte(Com, Ch);
}


VOID
SerWriteByte(
    _In_ BYTE Character
)
{
    _SerWritePort(COM_DEFAULT, Character);
}


VOID
SerWriteData(
    _In_ PBYTE Data,
    _In_ SIZE_T Size
)
{
#define NEW_LINE        '\n'
#define CARRIAGE_RETURN '\r'
#define TAB             '\t'

    if (!Data)
    {
        return;
    }

    for (SIZE_T index = 0; index < Size; index++)
    {
        switch (Data[index])
        {
        case NEW_LINE:
        case CARRIAGE_RETURN:
            _SerWritePort(COM_DEFAULT, NEW_LINE);
            break;

        default:
            _SerWritePort(COM_DEFAULT, Data[index]);
            break;
        }
    }
}
