#ifndef _ACPITABLES_H_
#define _ACPITABLES_H_

// If this is defined, we will always use the RSDT, even if the XSDT is available
#define ACPI_ALWAYS_USE_RSDT            0

#define ACPI_RSDP_SIGNATURE             "RSD PTR "

#define ACPI_OEM_ID_SIZE                6
#define ACPI_OEM_TABLE_ID_SIZE          8
#define ACPI_NAME_SIZE                  4

//
// Signatures
//
#define ACPI_SIG_BERT       "BERT"      // Boot Error Record Table
#define ACPI_SIG_CPEP       "CPEP"      // Corrected Platform Error Polling Table
#define ACPI_SIG_ECDT       "ECDT"      // Embedded Controller Boot Resources Table
#define ACPI_SIG_EINJ       "EINJ"      // Error Injection Table
#define ACPI_SIG_ERST       "ERST"      // Error Record Serialization Table
#define ACPI_SIG_HEST       "HEST"      // Hardware Error Source Table
#define ACPI_SIG_MADT       "APIC"      // Multiple APIC Description Table
#define ACPI_SIG_MSCT       "MSCT"      // Maximum System Characteristics Table
#define ACPI_SIG_SBST       "SBST"      // Smart Battery Specification Table
#define ACPI_SIG_SLIT       "SLIT"      // System Locality Distance Information Table
#define ACPI_SIG_SRAT       "SRAT"      // System Resource Affinity Table


//
// ACPI Tables (not all, just the one we need)
//

#pragma pack(push)
#pragma pack(1)

typedef struct _ACPI_TABLE_HEADER
{
    CHAR                    Signature[ACPI_NAME_SIZE];          // Table signature
    DWORD                   Length;                             // Size of table in bytes, including this header
    BYTE                    Revision;                           // ACPI revision
    BYTE                    Checksum;                           // Sum of entire table must be 0
    CHAR                    OemId[ACPI_OEM_ID_SIZE];            // OEM identification
    CHAR                    OemTableId[ACPI_OEM_TABLE_ID_SIZE]; // OEM table identification
    DWORD                   OemRevision;                        // OEM revision number
    CHAR                    AslCompilerId[ACPI_NAME_SIZE];      // ASCII ASL compiler vendor ID
    DWORD                   AslCompilerRevision;                // ASL compiler version
} ACPI_TABLE_HEADER, *PACPI_TABLE_HEADER;

typedef struct _RSDP_TABLE
{
    CHAR                    Signature[8];               // ACPI signature (ACPI_RSDP_SIGNATURE)
    BYTE                    Checksum;                   // ACPI 1.0 checksum
    CHAR                    OemId[ACPI_OEM_ID_SIZE];    // OEM identification
    BYTE                    Revision;                   // Must be (0) for ACPI 1.0 or (2) for ACPI 2.0+
    DWORD                   RsdtPhysicalAddress;        // 32-bit physical address of the RSDT
    DWORD                   Length;                     // Table length in bytes, including header (ACPI 2.0+)
    QWORD                   XsdtPhysicalAddress;        // 64-bit physical address of the XSDT (ACPI 2.0+)
    BYTE                    ExtendedChecksum;           // Checksum of entire table (ACPI 2.0+)
    BYTE                    Reserved[3];                // Reserved, must be zero */
} RSDP_TABLE, *PRSDP_TABLE;

typedef struct _RSDT_TABLE
{
    ACPI_TABLE_HEADER       Header;                     // Common header
    DWORD                   TableOffsetEntry[1];        // Array of pointers to other ACPI tables
} RSDT_TABLE, *PRSDT_TABLE;

typedef struct _SUBTABLE_HEADER
{
    BYTE                    Type;
    BYTE                    Length;
} SUBTABLE_HEADER, *PSUBTABLE_HEADER;

typedef struct _XSDT_TABLE
{
    ACPI_TABLE_HEADER       Header;                     // Common header
    QWORD                   TableOffsetEntry[1];        // Array of pointers to other ACPI tables
} XSDT_TABLE, *PXSDT_TABLE;


//
// MADT Types
//
#define MADT_TYPE_LOCAL_APIC               0x00
#define MADT_TYPE_IO_APIC                  0x01
#define MADT_TYPE_INTERRUPT_OVERRIDE       0x02
#define MADT_TYPE_NMI_SOURCE               0x03
#define MADT_TYPE_LOCAL_APIC_NMI           0x04
#define MADT_TYPE_LOCAL_APIC_OVERRIDE      0x05
#define MADT_TYPE_IO_SAPIC                 0x06
#define MADT_TYPE_LOCAL_SAPIC              0x07
#define MADT_TYPE_INTERRUPT_SOURCE         0x08
#define MADT_TYPE_LOCAL_X2APIC             0x09
#define MADT_TYPE_LOCAL_X2APIC_NMI         0x0A
#define MADT_TYPE_GENERIC_INTERRUPT        0x0B
#define MADT_TYPE_GENERIC_DISTRIBUTOR      0x0C
#define MADT_TYPE_RESERVED                 0x0D


typedef struct _MADT_LOCAL_APIC_TABLE
{
    SUBTABLE_HEADER         Header;
    BYTE                    ProcessorId;                // ACPI Processor ID
    BYTE                    Id;                         // Local APIC ID
    DWORD                   LapicFlags;
} MADT_LOCAL_APIC_TABLE, *PMADT_LOCAL_APIC_TABLE;

#pragma pack(pop)


BYTE
AcpiGetTableChecksum(
    _In_ PBYTE Buffer,
    _In_ DWORD Length
);

NTSTATUS
AcpiFindRootPointer(
    _Out_ QWORD *TableAddress
);

VOID
AcpiDumpRsdp(
    _In_ PRSDP_TABLE Rsdp
);

VOID
AcpiDumpTableHeader(
    _In_ PACPI_TABLE_HEADER Header
);

NTSTATUS
AcpiParseMadt(
    _In_ QWORD MadtPhysicalAddress,
    _In_ SIZE_T Size
);

NTSTATUS
AcpiParseXRsdt(
    _In_ QWORD TablePhysicalAddress,
    _In_ BOOLEAN Extended
);

//
// APIC Base MSR bits
//
#define APIC_BASE_CPU_IS_BSP        BIT(8)
#define APIC_BASE_GLB_EBABLED       BIT(11)
#define APIC_BASE_MASK              0xFEE00000ULL

#endif // !_ACPITABLES_H_
