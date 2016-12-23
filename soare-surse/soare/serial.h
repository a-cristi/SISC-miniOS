// Author: Cristi Anichitei, December 2015

#ifndef _SERIAL_H_
#define _SERIAL_H_

#define COM1                0x3F8
#define COM2                0x2F8
#define COM3                0x3E8
#define COM4                0x2E8

#define PORT_COM1           0
#define PORT_COM2           1
#define PORT_COM3           2
#define PORT_COM4           3
#define PORT_COM_LAST       3
#define PORT_COM_DEFAULT    (WORD)-1

/*
IO Port Offset      Setting of DLAB     Register mapped to this port
+0                  0                   Data register; read from / write to when communicating
+1                  0                   Interrupt Enable Register
+0                  1                   With DLAB set to 1, this is the least significant byte of the divisor value for setting the baud rate.
+1                  1                   With DLAB set to 1, this is the most significant byte of the divisor value.
+2                  -                   Interrupt Identification and FIFO control registers
+3                  -                   Line Control Register. The most significant bit of this register is the DLAB.
+4                  -                   Modem Control Register.
+5                  -                   Line Status Register.
+6                  -                   Modem Status Register.
+7                  -                   Scratch Register

*/

NTSTATUS
IoSerialInitPort(
    _In_ WORD Port,
    _In_ BOOLEAN SetDefault
);

VOID
IoSerialPreInit(
    VOID
);

SIZE_T
SerialReadString(
    _Out_ CHAR * Buffer,
    _In_ SIZE_T BufferSize
);

VOID
SerialPutString(
    _In_ PCHAR Str,
    _In_ SIZE_T Elements
);

#endif // _SERIAL_H_
