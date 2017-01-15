#ifndef _ACPITABLES_H_
#define _ACPITABLES_H_

#define ACPI_RSDP_SIGNATURE             "RSD PTR "

#define ACPI_OEM_ID_SIZE                6
#define ACPI_OEM_TABLE_ID_SIZE          8

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

#endif // !_ACPITABLES_H_
