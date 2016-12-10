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

#pragma pack(pop)

#define IDT_ENTRIES     256

// GDT selectors
#define GDT_CODEK_SELECTOR  0x08


NTSTATUS
ExInitExceptionHandling(
    _Inout_ INTERRUPT_GATE *Idt,
    _Inout_ IDTR *Idtr
);

#endif // !_DTR_H_
