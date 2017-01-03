#include "defs.h"
#include "ntstatus.h"
#include "keyboard.h"
#include "dtr.h"
#include "pic.h"
#include "log.h"
#include "debugger.h"
#include "panic.h"

//
// This is a very simple keyboard "driver". It should probably be designed as a state machine with a command queue.
// Also, we assume that the keyboard is present and enabled, and simply do a self test and an interface test, but the
// enable sequence and the checks should be done in a better way
//

//
// Keyboard ports
//
#define KB_ENC_REG_IN_BUFFER        0x60    // Read: read input buffer; Write: write command
#define KB_ENC_REG_CMD              0x60    // same as above, defined with another name for clarity
#define KB_CTRL_REG_STATUS          0x64    // Read: read status register; Write: write command
#define KB_CTRL_REG_CMD             0x64    // same as above, defined with another name for clarity

//
// Status register configuration
//
#define KB_STATUS_OUT_BUFFER        BIT(0)  // 0: output buffer empty (don't read); 1: output buffer full (can be read)
#define KB_STATUS_IN_BUFFER         BIT(1)  // 0: input buffer empty (can be written); 1: input buffer full (don't write)
#define KB_STATYS_SYS_FLAG          BIT(2)  // 0: after power/reset; 1: after self-test success
#define KB_STATUS_CMD_DATA          BIT(3)  // 0: last write was data (port 0x60); 1: last write was a command (port 0x64)
#define KB_STATUS_LOCKED            BIT(4)  // 0: locked; 1: not locked
#define KB_STATUS_AUX_FULL          BIT(5)  // 
#define KB_STATUS_TIMEOUT           BIT(6)  // 0: OK; 1: timeout
#define KB_STATUS_PARITY_ERROR      BIT(7)  // 0: OK; 1: parity error

//
// Encoder commands
//
#define ENC_CMD_SET_LEDS            0xED    // set LEDs
#define ENC_CMD_ECHO                0xEE    // returns 0xEE to port 0x60 as a diagnostic test
#define ENC_CMD_ALT_SCANCODE_SET    0xF0    // set alternate scan code set
#define ENC_CMD_KB_ID               0xF2    // send 2 byte keyboard ID code as the next 2 bytes to be read from port 0x60
#define ENC_CMD_SET_REPEAT_RATE     0xF3    // set autorepeat delay and repeat rate
#define ENC_CMD_ENABLE_KB           0xF4    // enable keyboard
#define ENC_CMD_RESET               0xF5    // reset to power on condition and wait for enable command
#define ENC_CMD_RESET_START_SCAN    0xF6    // reset to power on condition and begin scanning keyboard
#define ENC_CMD_SET_AUTOREPEAT      0xF7    // set all keys to autorepeat (PS/2 only)
#define ENC_CMD_SET_MAKE_BRAKE      0xF8    // set all keys to send make code and break code (PS/2 only)
#define ENC_CMD_ONLY_MAKE           0xF9    // set all keys to generate only make codes
#define ENC_CMD_AUTO_MAKE_BRAKE     0xFA    // set all keys to autorepeat and generate make/brake codes
#define ENC_CMD_SET_KEY_AUTOREPEAT  0xFB    // set a single key to autorepeat
#define ENC_CMD_SET_KEY_MAKE_BREAK  0xFC    // set a single key to generate make/brake codes
#define ENC_CMD_SET_KEY_BREAK       0xFD    // set a single key to generate only break codes
#define ENC_CMD_RESEND_RESULT       0xFE    // resend last result
#define ENC_CMD_RESET_START_TEST    0xFF    // reset to power on state and start self test

//
// Set LEDs command
//
// After 0xED is sent, the next byte written to port 0x60 updates the LEDs
#define LED_SCROLL_LOCK             BIT(0)  // 0: off; 1: on
#define LED_NUM_LOCK                BIT(1)  // 0: off; 1: on
#define LED_CAPS_LOCK               BIT(2)  // 0: off; 1: on

//
// Set autorepeat delay and repeat rate command
//
// After 0xF3 is send, the next byte written to port 0x60 has the following format
#define MAKE_REPEAT(rate, delay)    ((((delay) & 0x3) << 5) | ((rate) & 0xF))
#define RATE_MAX_CH_SEC             0x00    // ~30 characters/second
#define RATE_MIN_CH_SEC             0x1F    // ~2  characters/second
#define DELAY_0_25                  0       // 1/4 seconds
#define DELAY_0_50                  1       // 1/2 seconds
#define DELAY_0_75                  2       // 3/4 seconds
#define DELAY_1                     3       // 1   second

//
// Encoder return values (usually scan codes, but it can also signal errors)
//
#define ENC_IS_PRESS_SCAN_CODE(c)   (((c) >= 0x01 && (c) <= 0x58) || ((c) <= 0x81 || (c) >= 0xD8))
#define ENC_BUFFER_OVERRUN          0x00    // internal buffer overrun
#define ENC_ACK                     0xFA    // command acknowledged
#define ENC_ECHO                    0xEE    // returned from the echo command
#define ENC_RESEND                  0xFE    // keyboard request for system to resend the last command

//
// Controller commands
//
#define CTRL_CMD_READ               0x20    // read command byte
#define CTRL_CMD_WRITE              0x60    // write command byte
#define CTRL_CMD_SELF_TEST          0xAA    // self test
#define CTRL_CMD_IFACE_TEST         0xAB    // interface test
#define CTRL_CMD_DISABLE_KB         0xAD    // disable keyboard
#define CTRL_CMD_ENABLE_KB          0xAE    // enable keyboard
#define CTRL_CMD_READ_IN_PORT       0xC0    // read input port
#define CTRL_CMD_READ_OUT_PORT      0xD0    // read output port
#define CTRL_CMD_WRITE_OUT_PORT     0xD1    // write output port
#define CTRL_CMD_READ_TEST          0xE0    // read test inputs
#define CTRL_CMD_SYSTEM_RESET       0xFE    // system reset
#define CTRL_CMD_DISABLE_MOUSE      0xA7    // disable mouse port
#define CTRL_CMD_ENABLE_MOUSE       0xA8    // enable mouse port
#define CTRL_CMD_TEST_MOUSE         0xA9    // test mouse port
#define CTRL_CMD_WRITE_MOUSE        0xD4    // write to mouse

//
// Command byte (0x20) configuration
//
#define CTRL_CMDBYTE_KB_INT_ENABLE      BIT(0)  // 0: disable keyboard IRQ; 1: enable keyboard IRQ
#define CTRL_CMDBYTE_MOUSE_INT_ENABLE   BIT(1)  // 0: disable mouse IRQ; 1: enable mouse IRQ; only EISA/PS2
#define CTRL_CMDBYTE_SYSTEM_FLAG        BIT(2)  // 0: cold reboot; 1: warm reboot (BAT completed)
#define CTRL_CMDBYTE_IGNORE_KB_LOCK     BIT(3)  // 0: no action; 1: force bit 4 of status register to 1 (not locked)
#define CTRL_CMDBYTE_KB_ENABLE          BIT(4)  // 0: enable keyboard; 1: disable keyboard
#define CTRL_CMDBYTE_MOUSE_ENABLE       BIT(5)  // 0: enable mouse; 1: disable mouse
#define CTRL_CMDBYTE_TRANSLATION        BIT(6)  // 0: no translation; 1: translate key scan codes
// bit 7 is unused and should be 0

//
// Controller self test return values (port 0x60)
//
#define CTRL_SELF_TEST_PASSED           0x55
#define CTRL_SELF_TEST_FAILED           0xFC

//
// Controller interface test return values (port 0x60)
//
#define CTRL_IFACE_TEST_SUCCESS         0x00
#define CTRL_IFACE_CLOCK_LINE_LOW       0x01    // keyboard clock line stuck low
#define CTRL_IFACE_CLOCK_LINE_HIGH      0x02    // keyboard clock line stuck high
#define CTRL_IFACE_DATA_LINE_HIGH       0x03    // keyboard data line stuck high
#define CTRL_IFACE_GENERAL_ERROR        0xFF


//
// Keyboard state
//

typedef enum _KB_STATE
{
    kbStateNormal = 0,
    kbStateShiftHeld,
    kbStateCtrlHeld,
    kbStateAltHeld,
    kbStateNumLock,
    kbStateCapsLock,
    kbStateShiftCaps,
    kbStateShiftNum,
    kbStateAltGr,
    kbStateShiftCtrl,
    kbStateCount,       // must be the last one, NOT a valid state
} KB_STATE, *PKB_STATE;

typedef struct _KB_CONTEXT
{
    BOOLEAN     Enabled;
    
    // state
    KB_STATE    State;

    // easier manipulation for special keys
    struct  
    {
        BYTE    ScrollLock : 1;
        BYTE    NumLock : 1;
        BYTE    CapsLock : 1;

        BYTE    Ctrl : 1;
        BYTE    Shift : 1;
        BYTE    Alt : 1;

        BYTE    _reserved : 2;
    };

    BOOLEAN     Error;

    BOOLEAN     BatFailed;
    BOOLEAN     DiagnosticFailed;
    
    BOOLEAN     ResendRequested;

    BOOLEAN     Extended;

    volatile CHAR   LastPrintableChar;
} KB_CONTEXT, *PKB_CONTEXT;

#include "kbdcodes.h"

static KB_CONTEXT gKbContext;


//
// ASM handler
//
extern VOID IsrHndKeyboard(VOID);


static __forceinline BYTE
_KbCtrlReadStatus(
    VOID
)
{
    return __inbyte(KB_CTRL_REG_STATUS);
}

static __inline VOID
KbCtrlSendCommand(
    _In_ BYTE Cmd
)
{
    // wait for the input buffer to be clear
    while (0 != (_KbCtrlReadStatus() & KB_STATUS_IN_BUFFER));

    __outbyte(KB_CTRL_REG_CMD, Cmd);
}


static __inline VOID
_KbEncSendCommand(
    _In_ BYTE Cmd
)
{
    // commands sent to the encoder are sent to the controller first, so make sure that the controller input buffer is clear
    while (0 != (_KbCtrlReadStatus() & KB_STATUS_IN_BUFFER));

    __outbyte(KB_ENC_REG_CMD, Cmd);
}


static __inline BYTE
_KbEncReadBuffer(
    VOID
)
{
    // make sure the output buffer is full
    while (0 == (_KbCtrlReadStatus() & KB_STATUS_OUT_BUFFER));

    return __inbyte(KB_ENC_REG_IN_BUFFER);
}


static __inline BYTE
_KbEncSendCommandAndGetResponse(
    _In_ BYTE Cmd
)
{
    INT16 retryCount = 3;

    do 
    {
        BYTE reply;

        _KbEncSendCommand(Cmd);

        reply = _KbEncReadBuffer();
        if (ENC_ACK == reply)
        {
            return reply;
        }
        else if (ENC_ECHO == reply && ENC_ECHO == Cmd)
        {
            return reply;
        }
        else if (ENC_RESEND != reply)
        {
            LogWithInfo("[KB] 0x%02x -> 0x%02x\n", Cmd, reply);
            return reply;
        }

        LogWithInfo("[KB] 0x%02x -> RESEND: %d\n", Cmd, retryCount);
        retryCount--;
    } while (retryCount > 0);

    LogWithInfo("[KB] 0x%02x -> TIMEOUT\n", Cmd);
    return 0xFF;
}


VOID
KbUpdateLeds(
    _In_ BOOLEAN ScrollLock,
    _In_ BOOLEAN NumLock,
    _In_ BOOLEAN CapsLock
)
{
    BYTE data = 0
        | (ScrollLock ? LED_SCROLL_LOCK : 0)
        | (NumLock ? LED_NUM_LOCK : 0)
        | (CapsLock ? LED_CAPS_LOCK : 0);

    //_KbEncSendCommand(ENC_CMD_SET_LEDS);
    //_KbEncSendCommand(data);
    _KbEncSendCommandAndGetResponse(ENC_CMD_SET_LEDS);
    _KbEncSendCommandAndGetResponse(data);
}


VOID
KbResetSystem(
    VOID
)
{
    KbCtrlSendCommand(CTRL_CMD_WRITE);
    _KbEncSendCommand(CTRL_CMD_SYSTEM_RESET);
}


VOID
KbHandler(
    _In_ PVOID Context
)
{
    BYTE code;

    UNREFERENCED_PARAMETER(Context);

    gKbContext.LastPrintableChar = 0;
    code = _KbEncReadBuffer();

    // check for extended
    if (scExt == code)
    {
        gKbContext.Extended = TRUE;
    }
    else
    {
        BOOLEAN bIsBreak = (code & 0x80);
        if (bIsBreak)
        {
            code -= 0x80;
        }

        // check LEDs (one press enables the mode, second press disables it)
        if (scCaps == code && bIsBreak)
        {
            gKbContext.CapsLock = !gKbContext.CapsLock;
            KbUpdateLeds(gKbContext.ScrollLock, gKbContext.NumLock, gKbContext.CapsLock);
        }
        else if (scNum == code && bIsBreak)
        {
            gKbContext.NumLock = !gKbContext.NumLock;
            KbUpdateLeds(gKbContext.ScrollLock, gKbContext.NumLock, gKbContext.CapsLock);
        }
        else if (scScroll == code && bIsBreak)
        {
            gKbContext.ScrollLock = !gKbContext.ScrollLock;
            KbUpdateLeds(gKbContext.ScrollLock, gKbContext.NumLock, gKbContext.CapsLock);
        }
        // check ctrl
        else if (scLCtrl == code)
        {
            gKbContext.Ctrl = !bIsBreak;
        }
        else if (gKbContext.Extended && scExtRCtrl == code)
        {
            gKbContext.Ctrl = !bIsBreak;
        }
        // check shift
        else if (scLShift == code)
        {
            gKbContext.Shift = !bIsBreak;
        }
        else if (scRShift == code)
        {
            gKbContext.Shift = !bIsBreak;
        }
        // check alt
        else if (scLAlt == code)
        {
            gKbContext.Alt = !bIsBreak;
        }
        else if (gKbContext.Extended && scExtRAlt == code)
        {
            gKbContext.Alt = !bIsBreak;
        }

        // update the current mode
        if (gKbContext.Shift && gKbContext.CapsLock)
        {
            gKbContext.State = kbStateShiftCaps;
        }
        else if (gKbContext.Shift && gKbContext.Ctrl)
        {
            gKbContext.State = kbStateShiftCtrl;
        }
        else if (gKbContext.Shift && gKbContext.NumLock)
        {
            gKbContext.State = kbStateShiftNum;
        }
        else if (gKbContext.Shift)
        {
            gKbContext.State = kbStateShiftHeld;
        }
        else if (gKbContext.Ctrl)
        {
            gKbContext.State = kbStateCtrlHeld;
        }
        else if (gKbContext.Alt)
        {
            gKbContext.State = kbStateAltHeld;
        }
        //else if (gKbContext.Alt)
        //{
        //    gKbContext.State = kbStateAltGr;
        //}
        else if (gKbContext.CapsLock)
        {
            gKbContext.State = kbStateCapsLock;
        }
        else if (gKbContext.NumLock)
        {
            gKbContext.State = kbStateNumLock;
        }
        else if (!gKbContext.Alt && !gKbContext.CapsLock && !gKbContext.Ctrl && !gKbContext.NumLock && !gKbContext.Shift)
        {
            gKbContext.State = kbStateNormal;
        }

        if (!bIsBreak)
        {
            CHAR key = gKbContext.Extended ? (gKbdExtCodes[code][gKbContext.State] & 0xFF) :
                (gKbdUsCodes[code][gKbContext.State] & 0xFF);

#define IsPrintable(c)      (((' ' <= (c)) && ((c) <= '~')) || ('\n' == (c)) || ('\r' == (c)) || ('\t' == (c)) || ('\b' == (c)))

            if (IsPrintable(key))
            {
                //Log("%c", key);
                gKbContext.LastPrintableChar = key;
            }
            else
            {
                LogWithInfo("[KB] Unknown or unprintable scan code 0x%02x in mode %d (extended: %d)\n",
                    code, gKbContext.State, gKbContext.Extended);
            }
        }

        gKbContext.Extended = FALSE;
    }
}


NTSTATUS
KbInit(
    VOID
)
{
    BYTE reply;
    NTSTATUS status = DtrInstallIrqHandler(IRQ2INTR(PIC_IRQ_KEYBOARD), IsrHndKeyboard);
    if (!NT_SUCCESS(status))
    {
        LogWithInfo("[ERROR] DtrInstallIrqHandler failed for 0x%04 -> %018p: 0x%08x\n", PIC_IRQ_KEYBOARD, IsrHndKeyboard, status);
        return status;
    }

    // do the self test
    KbCtrlSendCommand(CTRL_CMD_SELF_TEST);
    reply = _KbEncReadBuffer();
    if (CTRL_SELF_TEST_PASSED != reply)
    {
        Log("[ERROR] Keyboard self test failed: 0x%02x\n", reply);
        return STATUS_INTERNAL_ERROR;
    }

    // do the interface test
    KbCtrlSendCommand(CTRL_CMD_IFACE_TEST);
    reply = _KbEncReadBuffer();
    if (CTRL_IFACE_TEST_SUCCESS != reply)
    {
        Log("[ERROR] Keyboard interface test failed: 0x%02x\n", reply);
        return STATUS_INTERNAL_ERROR;
    }

    // echo
    if (ENC_CMD_ECHO != _KbEncSendCommandAndGetResponse(ENC_CMD_ECHO))
    {
        Log("[ERROR] Keyboard did not reply with 0xEE to ECHO Encoder command!\n");
        return STATUS_INTERNAL_ERROR;
    }

    gKbContext.State = kbStateNormal;
    gKbContext.ScrollLock = gKbContext.NumLock = gKbContext.CapsLock = FALSE;
    gKbContext.Ctrl = FALSE;
    gKbContext.Shift = FALSE;
    gKbContext.Alt = FALSE;
    gKbContext.BatFailed = gKbContext.DiagnosticFailed = gKbContext.Error = gKbContext.ResendRequested = FALSE;
    gKbContext.Enabled = TRUE;
    gKbContext.Extended = FALSE;
    gKbContext.LastPrintableChar = 0;

    // play with the LEDs so the keyboard will know who is its new master!
    KbUpdateLeds(FALSE, FALSE, FALSE);
    KbUpdateLeds(TRUE, TRUE, TRUE);
    KbUpdateLeds(gKbContext.ScrollLock, gKbContext.NumLock, gKbContext.CapsLock);

    // get the Keyboard ID
    _KbEncSendCommandAndGetResponse(ENC_CMD_KB_ID);
    LogWithInfo("[KB] ID: 0x%02x 0x%02x\n", _KbEncReadBuffer(), _KbEncReadBuffer());

    // and let the IRQs come!
    PicEnableIrq(PIC_IRQ_KEYBOARD);

    return STATUS_SUCCESS;
}


CHAR
KbGetCh(
    VOID
)
{
    CHAR ch;

    // wait for a printable char
    do 
    {
        ch = gKbContext.LastPrintableChar;
    } while (ch == 0);

    // consume it
    gKbContext.LastPrintableChar = 0;

    return ch;
}
