#include "defs.h"
#include "ntstatus.h"
#include "serial.h"

#define NEW_LINE            '\n'
#define CARRIAGE_RETURN     '\r'
#define TAB                 '\t'
#define NULL_TERMINATOR     '\0'
#define BACKSPACE           '\b'

typedef struct _COM_PORT
{
    WORD        Port;
    BOOLEAN     Inited;
} COM_PORT, *PCOM_PORT;

PCOM_PORT gDefaultPort = NULL;

COM_PORT gSerialPorts[] = {
    // COM1
    {
        COM1,       // Port
        FALSE       // Inited
    },
    // COM2
    {
        COM2,       // Port
        FALSE       // Inited
    },
    // COM3
    {
        COM3,       // Port
        FALSE       // Inited
    },
    // COM4
    {
        COM4,       // Port
        FALSE       // Inited
    }
};


NTSTATUS
IoSerialInitPort(
    _In_ WORD Port,  // PORT_COM*
    _In_ BOOLEAN SetDefault
)
{
    PCOM_PORT pPort = NULL;

    if (Port > PORT_COM_LAST)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    pPort = &gSerialPorts[Port];

    if (pPort->Inited)
    {
        return STATUS_ALREADY_INITIALIZED;
    }

    // disable interrupts
    __outbyte(pPort->Port + 1, 0x0);

    // enable DLAB
    __outbyte(pPort->Port + 3, 0x80);

    // divisor low byte (baud rate 115200)
    __outbyte(pPort->Port + 0, 0x01);

    // divisor high byte
    __outbyte(pPort->Port + 1, 0x00);

    // 8N1 (8 bits, no parity, 1 stop bit)
    __outbyte(pPort->Port + 3, 0x03);

    // enable FIFO
    __outbyte(pPort->Port + 2, 0xC7);

    // enable IRQs
    __outbyte(pPort->Port + 4, 0x0B);

    // should we use this as the default port?
    if (SetDefault)
    {
        gDefaultPort = pPort;
    }

    return STATUS_SUCCESS;
}


VOID
IoSerialPreInit(
    VOID
)
{
    // nothing to do for now
}


static __forceinline INT32
_SerRcvd(
    _In_ WORD PortAddress   // COM*, NOT PORT_COM* !!
)
{
    return __inbyte(PortAddress + 5) & 1;
}


__inline CHAR
SerialReadPort(
    __in WORD PortAddress
)
{
    while (!_SerRcvd(PortAddress));

    return __inbyte(PortAddress);
}


CHAR
SerialRead(
    VOID
)
{
    return SerialReadPort(gDefaultPort->Port);
}


static __forceinline INT32
_SerIsTransmitEmpty(
    _In_ WORD PortAddress
)
{
    return __inbyte(PortAddress + 5) & 0x20;
}


__inline VOID
SerialWritePort(
    _In_ WORD PortAddress,
    _In_ CHAR Character
)
{
    while (!_SerIsTransmitEmpty(PortAddress));

    __outbyte(PortAddress, Character);
}


VOID
SerialWrite(
    _In_ CHAR Character
)
{
    SerialWritePort(gDefaultPort->Port, Character);
}


__inline VOID
SerialPutStringToPort(
    _In_ WORD PortAddress,
    _In_ PCHAR Str,
    _In_ SIZE_T Elements
)
{
    SIZE_T index = 0;

    for (index = 0; index < Elements; index++)
    {
        switch (Str[index])
        {
        case NEW_LINE:
        case CARRIAGE_RETURN:
            SerialWritePort(PortAddress, '\n');
            break;
            
        case NULL_TERMINATOR:
            return;

            // ignore backspaces
        case BACKSPACE:
            continue;

        default:
            SerialWritePort(PortAddress, Str[index]);
            break;
        }
    }
}


VOID
SerialPutString(
    _In_ PCHAR Str,
    _In_ SIZE_T Elements
)
{
    if (Str)
    {
        SerialPutStringToPort(gDefaultPort->Port, Str, Elements);
    }
}


__inline SIZE_T
SerialReadStringFromPort(
    _In_ WORD PortAddress,
    _Out_ CHAR * Buffer,
    _In_ SIZE_T BufferSize
)
{
    SIZE_T i = 0;

    BufferSize -= 1;

    for (i = 0; i < BufferSize; i++)
    {
        CHAR ch = SerialReadPort(PortAddress);

        switch (ch)
        {
        case NEW_LINE:
        case CARRIAGE_RETURN:
            Buffer[i] = '\0';
            goto _exit;

        default:
            Buffer[i] = ch;
            break;
        }
    }

_exit:
    // Add a zero at the end
    Buffer[i] = '\0';
    i++;

    return i;
}


SIZE_T
SerialReadString(
    _Out_ CHAR * Buffer,
    _In_ SIZE_T BufferSize
)
{
    if (!Buffer)
    {
        return 0;
    }

    return SerialReadStringFromPort(gDefaultPort->Port, Buffer, BufferSize);
}
