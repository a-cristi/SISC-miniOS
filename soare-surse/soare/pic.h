#ifndef _PIC_H_
#define _PIC_H_

// See http://www.brokenthorn.com/Resources/OSDevPic.html

#define PIC1_CMD_PORT       0x20    // write-only
#define PIC1_DATA_PORT      0x21

#define PIC2_CMD_PORT       0xA0    // write-only
#define PIC2_DATA_PORT      0xA1

#define PIC1_BASE           0x20    // where to map IRQs 0-7
#define PIC2_BASE           0x28    // where to map IRQs 8-15

#define PIC_EOI             0x20

//
// ICWs (Initialization Control Words)
//
#define PIC_ICW1_ICW4       0x01    // if set: the PIC expects to receive ICW4 during initialization
#define PIC_ICW1_SINGLE     0x02    // if set: the system has only one PIC; else: the PIC is cascaded; ICW3 must be sent
#define PIC_ICW1_INTERVAL4  0x04    // if set: CALL address interval is 4; else: 8; usually ignored and cleared
#define PIC_ICW1_LEVEL      0x08    // if set: trigger mode is level; else: edge
#define PIC_ICW1_INIT       0x10    // if set: the PIC is to be initialized
// Bits [5:7] are reserved and must be 0

#define PIC_ICW4_8086       0x01    // if set: 80x86 mode; else: MCS-80/86 mode
#define PIC_ICW4_AUTO_EOI   0x02    // if set: the controller automatically performs an EOI on the last interrupt acknowledge pulse
#define PIC_ICW4_BUF_MS     0x04    // valid only if BUF is set; if set: selects buffer master; else: selects buffer slave
#define PIC_ICW4_BUF        0x08    // if set: operated in buffered mode
#define PIC_ICW4_SFNM       0x10    //
// Bits [5:7] are reserved and must be 0

//
// OCWs (Operation Control Words)
//
#define PIC1_IMR_PORT       0x21    // read-only; IMR (Interrupt Mask Register) 
#define PIC2_IMR_PORT       0xA1    // read-only; IMR (Interrupt Mask Register) 

// Interrupt level
#define PIC_OCW2_L0         0x01
#define PIC_OCW2_L1         0x02
#define PIC_OCW2_L2         0x04
#define PIC_OCW2_L_MASK     0x07
// Bits [3:4] are reserved and must be 0
#define PIC_OCW2_EOI        0x20    // End of interrupt
#define PIC_OCW2_SL         0x40    // Selection
#define PIC_OCW2_R          0x80    // Rotation option

#define PIC_OCW3_RIS        0x01
#define PIC_OCW3_RIR        0x02
#define PIC_OCW3_MODE       0x04
#define PIC_OCW3_SMM        0x20
#define PIC_OCW3_ESMM       0x40
#define PIC_OCW3_MASK_D7    0x80

VOID
PicInitialize(
    VOID
);

VOID
PicDisable(
    VOID
);

#endif // !_PIC_H_
