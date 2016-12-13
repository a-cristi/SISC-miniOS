#include "defs.h"
#include "ntstatus.h"
#include "dtr.h"
#include "panic.h"
#include "pic.h"
#include "debugger.h"
#include "log.h"


//
// PIT Registers
//
#define PIT_REG_COUNTER0        0x40
#define PIT_REG_COUNTER1        0x41
#define PIT_REG_COUNTER2        0x42
#define PIT_REG_CMD             0x43

//
// Control word configuration
//
#define PIT_CMD_BINARY_COUNTER      0       // Bit 0; 0: Binary; 1: Binary Coded Decimal
#define PIT_CMD_MODE_MASK           0xE
#define PIT_CMD_MODE_COUNTDOWN      0x0     // 000: Interrupt on terminal count
#define PIT_CMD_MODE_ONE_SHOT       0x2     // 001: One shot
#define PIT_CMD_MODE_RATE_GEN       0x4     // 010: Rate generator
#define PIT_CMD_MODE_SQ_WAVE_GEN    0x6     // 011: Square wave generator
#define PIT_CMD_MODE_SW_STROBE      0x8     // 100: Software triggered strobe
#define PIT_CMD_MODE_HW_STROBE      0xA     // 101: Hardware triggered strobe
#define PIT_CMD_RL_MASK             0x30
#define PIT_CMD_RL_LATCHED          0x0
#define PIT_CMD_RL_LSB_ONLY         0x10
#define PIT_CMD_RL_MSB_ONLY         0x20
#define PIT_CMD_RL_LSB_THEN_MSB     0x30
#define PIT_CMD_COUNTER_MASK        0xC0
#define PIT_CMD_COUNTER0            0x0
#define PIT_CMD_COUNTER1            0x40
#define PIT_CMD_COUNTER2            0x80


volatile SIZE_T gPitTickCount;
static BOOLEAN gPitInited;

extern VOID IsrHndPic(VOID);

static
__inline
SIZE_T
_PitSetTickCount(
    _In_ SIZE_T Ticks
)
{
    SIZE_T currentCount = gPitTickCount;
    gPitTickCount = Ticks;

    return currentCount;
}


static
__inline
VOID
_PitSendCommand(
    _In_ BYTE Cmd
)
{
    __outbyte(PIT_REG_CMD, Cmd);
}


static
__inline
VOID
_PitSendData(
    _In_ BYTE Counter,
    _In_ BYTE Data
)
{
    __outbyte(Counter, Data);
}


static
__inline
BYTE
_PitReceiveData(
    _In_ BYTE Counter
)
{
    return __inbyte(Counter);
}


static
VOID
_PitStart(
    _In_ WORD Frequency,
    _In_ BYTE Counter,
    _In_ BYTE Mode
)
{
    WORD divisor = (WORD)(1193181 / Frequency);
    BYTE ocw = 0;

    ocw |= (Mode & PIT_CMD_MODE_MASK);
    ocw |= PIT_CMD_RL_LSB_THEN_MSB;
    ocw |= (Counter & PIT_CMD_COUNTER_MASK);
    _PitSendCommand(ocw);

    _PitSendData(PIT_CMD_COUNTER0 == Counter ? PIT_REG_COUNTER0 : PIT_REG_COUNTER2, divisor & 0xFF);
    _PitSendData(PIT_CMD_COUNTER0 == Counter ? PIT_REG_COUNTER0 : PIT_REG_COUNTER2, (divisor >> 8) & 0xFF);
    gPitTickCount = 0;
}


VOID
PitHandler(
    VOID
)
{
    gPitTickCount++;
}


NTSTATUS
TmrInitializeTimer(
    VOID
)
{
    NTSTATUS status = DtrInstallIrqHandler(IRQ2INTR(PIC_IRQ_TIMER), IsrHndPic);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] DtrInstallIrqHandler failed for 0x%04 -> %018p: 0x%08x\n", PIC_IRQ_TIMER, IsrHndPic, status);
        return status;
    }

    {
        DWORD divisor = 1193180 / 200;      // Fire 5965 times/s
        __outbyte(0x43, 0x36);
        __outbyte(0x40, divisor & 0xFF);
        __outbyte(0x40, divisor >> 8);
    }
    PicEnableIrq(PIC_IRQ_TIMER);
    //_PitStart(100, PIT_CMD_COUNTER0, PIT_CMD_MODE_RATE_GEN);

    return STATUS_SUCCESS;
}
