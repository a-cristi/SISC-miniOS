#ifndef _WINLISTS_H_
#define _WINLISTS_H_

//
// WDK lists
//

typedef struct _LIST_ENTRY
{
    union
    {
        struct _LIST_ENTRY *Flink;
        struct _LIST_ENTRY *Head;
    };
    union
    {
        struct _LIST_ENTRY *Blink;
        struct _LIST_ENTRY *Tail;
    };
} LIST_ENTRY, LIST_HEAD, *PLIST_ENTRY, *PLIST_HEAD;

#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field)     ((type *)((PCHAR)(address) - (SIZE_T)(&((type *)0)->field)))
#endif

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)                   ((QWORD)&(((type *)0)->field))
#endif

#ifndef FIELD_SIZE
#define FIELD_SIZE(type, field)                     (sizeof(((type *)0)->field))
#endif


__forceinline 
VOID
InitializeListHead(
    _Inout_ PLIST_ENTRY ListHead
)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

__forceinline 
BOOLEAN
IsListEmpty(
    _In_ const LIST_ENTRY * ListHead
)
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

__forceinline 
BOOLEAN
RemoveEntryList(
    _In_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

__forceinline 
PLIST_ENTRY
RemoveHeadList(
    _Inout_ PLIST_ENTRY ListHead
)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}

__forceinline 
PLIST_ENTRY
RemoveTailList(
    _Inout_ PLIST_ENTRY ListHead
)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}

__forceinline 
VOID
InsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}

__forceinline 
VOID
InsertHeadList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}

__forceinline VOID 
InsertAfterList(
    _Inout_ PLIST_ENTRY Pivot,
    _Inout_ PLIST_ENTRY Item
)
{
    Pivot->Flink->Blink = Item;
    Item->Flink = Pivot->Flink;
    Pivot->Flink = Item;
    Item->Blink = Pivot;
}

#endif // !_WINLISTS_H_
