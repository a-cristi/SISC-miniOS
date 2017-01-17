#include "defs.h"
#include "memdefs.h"
#include "ntstatus.h"
#include "kpool.h"
#include "winlists.h"
#include "log.h"

#pragma pack(push)
#pragma pack(1)

typedef struct _KPOOL_HEADER
{
    LIST_ENTRY  Link;
} KPOOL_HEADER, *PKPOOL_HEADER;

#pragma pack(pop)


static LIST_HEAD gKpListHead;


NTSTATUS
KpInit(
    _In_ PVOID Base,
    _In_ DWORD Length,
    _In_ DWORD EntrySize
)
{
    DWORD pages;

    if (PAGE_SIZE_4K == EntrySize)
    {
        pages = SMALL_PAGE_COUNT(Length);
    }
    else if (PAGE_SIZE_2M == EntrySize)
    {
        pages = LARGE_PAGE_COUNT(Length);
    }
    else
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (Length < EntrySize)
    {
        return STATUS_NO_MEMORY;
    }

    if (!Base || (QWORD)Base % EntrySize)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (Length % EntrySize)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    InitializeListHead(&gKpListHead);

    for (DWORD p = 0; p < pages; p++)
    {
        PKPOOL_HEADER pHeader = (KPOOL_HEADER *)((SIZE_T)Base + p * EntrySize);

        memset(pHeader, 0, sizeof(KPOOL_HEADER));
        InsertTailList(&gKpListHead, &pHeader->Link);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
KpAlloc(
    _Out_ PVOID *Ptr
)
{
    PKPOOL_HEADER pHeader;

    if (!Ptr)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (IsListEmpty(&gKpListHead))
    {
        return STATUS_NO_MEMORY;
    }

    pHeader = CONTAINING_RECORD(gKpListHead.Flink, KPOOL_HEADER, Link);
    RemoveEntryList(&pHeader->Link);
    memset(pHeader, 0, sizeof(KPOOL_HEADER));

    *Ptr = (VOID *)pHeader;

    return STATUS_SUCCESS;
}


NTSTATUS
KpFreeAndNull(
    _Inout_ PVOID *Ptr
)
{
    PKPOOL_HEADER pHeader;

    if (!Ptr || !*Ptr)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    pHeader = (KPOOL_HEADER *)*Ptr;
    memset(pHeader, 0, sizeof(KPOOL_HEADER));
    InsertTailList(&gKpListHead, &pHeader->Link);

    *Ptr = NULL;

    return STATUS_SUCCESS;
}
