#ifndef _DTR_H_
#define _DTR_H_

#pragma pack(push)
#pragma pack(1)

typedef struct _TSS64
{
    DWORD       Reserved1;

    QWORD       Rsp0;
    QWORD       Rsp1;
    QWORD       Rsp2;

    QWORD       Reserved2;

    QWORD       Ist1;
    QWORD       Ist2;
    QWORD       Ist3;
    QWORD       Ist4;
    QWORD       Ist5;
    QWORD       Ist6;
    QWORD       Ist7;

    QWORD       Reserved3;
    WORD        Reserved4;

    WORD        IoMapBase;
} TSS64, *PTSS64;

typedef union _INTERRUPT_GATE
{
    struct
    {
        WORD    Offset_15_00;
        WORD    Selector;

        union
        {
            struct
            {
                WORD    Ist : 3;
                WORD    _Zeroes : 5;
                WORD    Type : 4;
                WORD    S : 1;
                WORD    DPL : 2;
                WORD    P : 1;
            };
            WORD        Fields;
        };

        WORD    Offset_31_16;
        DWORD   Offset_63_32;
        DWORD   _Reserved;
    };

    QWORD       Raw[2];
} INTERRUPT_GATE, *PINTERRUPT_GATE;

#define IDT_ENTRIES     256

typedef union _SYSTEM_DESCRIPTOR
{
    struct
    {
        WORD    Limit_15_00;
        WORD    Base_15_00;
        BYTE    Base_23_16;

        union
        {
            struct
            {
                WORD    Type : 4;
                WORD    S : 1;
                WORD    DPL : 2;
                WORD    P : 1;
                WORD    Limit_19_16 : 4;
                WORD    AVL : 1;
                WORD    _Reserved : 2;
                WORD    G : 1;
            };
            WORD    Fields;
        };

        BYTE    Base_31_24;
        DWORD   Base_63_32;
        DWORD   MustBeZero;
    };
} SYSTEM_DESCRIPTOR, *PSYSTEM_DESCRIPTOR;

typedef struct _DTR
{
    WORD        Size;
    QWORD       Address;
} DTR, *PDTR;

typedef DTR     GDTR, *PGDTR;
typedef DTR     IDTR, *PIDTR;
typedef DTR     LDTR, *PLDTR;

// Defined like this for simplicity, keep in sync with what we actually use as the GDT
typedef struct _GDT_LAYOUT
{
    QWORD               NullDescriptor; // 0x00
    QWORD               KCode64;        // 0x08
    QWORD               KData64;        // 0x10
    QWORD               _Padding0;
    SYSTEM_DESCRIPTOR   KGs;            // 0x20
    SYSTEM_DESCRIPTOR   Tss;            // 0x30
} GDT_LAYOUT, *PGDT_LAYOUT;

typedef struct _PCPU
{
    struct _PCPU *  Self;
    DWORD           ApicId; // MADT.LocalApicId
    DWORD           Number; // in initialization order
    BOOLEAN         IsBsp;
    BYTE            _Padding[7];

    IDTR            Idtr;
    BYTE            _IdtrPadding[6];
    GDTR            Gdtr;
    BYTE            _GdtrPadding[4];
    DTR             Tr;

    INTERRUPT_GATE  Idt[IDT_ENTRIES];
    GDT_LAYOUT      Gdt;
    TSS64           Tss;
} PCPU, *PPCPU;

#pragma pack(pop)

#define MAX_CPU_COUNT       16

static_assert(sizeof(INTERRUPT_GATE) * IDT_ENTRIES <= 4096, "Invalid IDT size!");

// GDT descriptors (again, for simplicity)
#define GDT_NULL_DESCRIPTOR         0x0000000000000000ULL
#define GDT_KCODE64_DESCRIPTOR      0x002F9A000000FFFFULL
#define GDT_KDATA64_DESCRIPTOR      0x00CF92000000FFFFULL

// GDT selectors
#define GDT_NULL_SELECTOR           0x00
#define GDT_KCODE64_SELECTOR        0x08
#define GDT_KDATA64_SELECTOR        0x10
#define GDT_KGS_SELECTOR            0x20
#define GDT_TSS_SELECTOR            0x30

#define SEL_TYPE_LDT            0x02
#define SEL_TYPE_RWA_DATA       0x03
#define SEL_TYPE_RM_TR          0x03
#define SEL_TYPE_RWA_DATA_ED    0x07
#define SEL_TYPE_PM_TR          0x0B
#define SEL_TYPE_ACC_CODE       0x0F
#define SEL_USABLE              0x10
#define SEL_PRESENT             0x80
#define SEL_UNUSABLE            0x10000

NTSTATUS
DtrInitAndLoadAll(
    _Inout_ PCPU *Cpu
);

NTSTATUS
DtrCreatePcpu(
    _Out_ PPCPU *Cpu
);

#endif // !_DTR_H_
