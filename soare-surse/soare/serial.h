#ifndef _SERIAL_H_
#define _SERIAL_H_

VOID
SerComInit(
    VOID
);

VOID
SerWriteByte(
    _In_ BYTE Character
);

VOID
SerWriteData(
    _In_ PBYTE Data,
    _In_ SIZE_T Size
);

#endif // !_SERIAL_H_
