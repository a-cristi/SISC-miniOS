#include "defs.h"
#include "pic.h"

BYTE gImrMaster = 0xFB;
BYTE gImrSlave = 0xFF;


static __forceinline
VOID
_PicOutByte(
    _In_ BYTE Port,
    _In_ BYTE Value
)
{
    __outbyte(Port, Value);
    __inbyte(0x80);
}


static __forceinline
VOID
_PicSetImr(
    _In_ BYTE Master,
    _In_ BYTE Slave
)
{
    _PicOutByte(PIC1_IMR_PORT, Master);
    _PicOutByte(PIC2_IMR_PORT, Slave);
}


static __forceinline
VOID
_PicRemapInterupts(
    _In_ BYTE MasterBase,
    _In_ BYTE SlaveBase
)
{
    _PicOutByte(PIC1_DATA_PORT, MasterBase);
    _PicOutByte(PIC2_DATA_PORT, SlaveBase);
}


VOID
PicInitialize(
    VOID
)
{
    // send ICW1 to both PICs
    _PicOutByte(PIC1_CMD_PORT, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    _PicOutByte(PIC2_CMD_PORT, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    // send ICW and remap IRQs
    _PicRemapInterupts(PIC1_BASE, PIC2_BASE);

    // send ICW3 to let the PICs know what IRQ lines to use when communicating with each other
    // inform the master PIC that the slave is on IRQ2
    _PicOutByte(PIC1_DATA_PORT, (1 << 2));
    // and inform the slave PIC that it will be on IRQ2
    _PicOutByte(PIC2_DATA_PORT, 2);

    // send ICW4: operate in 80x86 mode
    _PicOutByte(PIC1_DATA_PORT, PIC_ICW4_8086);
    _PicOutByte(PIC2_DATA_PORT, PIC_ICW4_8086);

    // mask everything
    PicDisable();
}


VOID
PicDisable(
    VOID
)
{
    _PicOutByte(PIC2_DATA_PORT, 0xFF);
    _PicOutByte(PIC1_DATA_PORT, 0xFF);
}


VOID
PicDisableIrq(
    _In_ BYTE IrqNumber
)
{
    if (IrqNumber < 8)
    {
        gImrMaster |= (1 << IrqNumber);
    }
    else
    {
        gImrSlave |= (1 << (IrqNumber - 8));
    }

    _PicSetImr(gImrMaster, gImrSlave);
}


VOID
PicEnableIrq(
    _In_ BYTE IrqNumber
)
{
    if (IrqNumber < 8)
    {
        gImrMaster &= ~(1 << IrqNumber);
    }
    else
    {
        gImrSlave &= ~(1 << (IrqNumber - 8));
    }

    _PicSetImr(gImrMaster, gImrSlave);
}


VOID
PicSendCommand(
    _In_ BYTE Cmd,
    _In_ BOOLEAN SlavePic
)
{
    _PicOutByte(SlavePic ? PIC2_CMD_PORT : PIC1_CMD_PORT, Cmd);
}
