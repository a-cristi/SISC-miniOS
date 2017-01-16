#ifndef _APIC_H_
#define _APIC_H_

//
// Local APIC Registers
//
#define LAPI_REG_ID             0x0020  // R/W
#define LAPIC_REG_VERSION       0x0030  // RO
#define LAPIC_REG_EOI           0x00B0
#define LAPIC_REG_LDR           0x00D0  // Logical Destination Register, R/W
#define LAPIC_REG_DFR           0x00E0  // Destination Format
#define LAPIC_REG_SPURIOUS      0x00F0
#define LAPIC_REG_ERROR_STS     0x0280
#define LAPIC_REG_ICR_LOW       0x0300
#define LAPIC_REG_ICR_HIGH      0x0310
#define LAPIC_REG_LVT_TMR       0x0320
#define LAPIC_REG_LVT_LINT0     0x0350
#define LAPIC_REG_LVT_LINT1     0x0360
#define LAPIC_REG_LVT_ERR       0x0370
#define LAPIC_REG_TMR_INIT      0x0380
#define LAPIC_REG_TMR_COUNT     0x0390
#define LAPIC_REG_TMR_DIV       0x03E0

//
// Delivery modes
//
#define ICR_DMODE_FIXED         0x00
#define ICR_DMODE_SMI           0x02
#define ICR_DMODE_NMI           0x04
#define ICR_DMODE_INIT          0x05
#define ICR_DMODE_SIPI          0x06

#define ICR_DESTINATION_BIT     0x0800
#define ICR_DELIVERY_STS_BIT    0x1000
#define ICR_LV_BIT              0x4000

NTSTATUS
ApicInit(
    VOID
);

DWORD
ApicReadRegister(
    _In_ DWORD Register
);

VOID
ApicWriteRegister(
    _In_ DWORD Register,
    _In_ DWORD Value
);

#endif // !_APIC_H_
