#include "defs.h"
#include "screen.h"

// Private data
typedef struct _SCREEN
{
    PWORD       Buffer;

    WORD        CurrentRow;
    WORD        CurrentColumn;

    BYTE        CurrentColorStyle;  // background and foreground information
} SCREEN, *PSCREEN;


static SCREEN gScreen = { 0 };


#define VGA_MAKE_STYLE(fg, bg)          ((BYTE)((fg) | ((bg) << 4)))
#define VGA_MAKE_ENTRY(ch, st)          ((WORD)((ch) | ((st) << 8)))

#define VGA_MAKE_OFFSET(row, column)    ((row) * VGA_COLUMNS + (column))


static VOID
_VgaScroll(
    VOID
)
{
    // move everything up
    for (WORD row = 0; row < gScreen.CurrentRow; row++)
    {
        for (WORD column = 0; column < VGA_COLUMNS; column++)
        {
            gScreen.Buffer[VGA_MAKE_OFFSET(row, column)] = gScreen.Buffer[VGA_MAKE_OFFSET(row + 1, column)];
        }
    }

    // and clear the last column using the current style
    for (WORD column = 0; column < VGA_COLUMNS; column++)
    {
        BYTE bg = gScreen.CurrentColorStyle >> 4;
        gScreen.Buffer[VGA_MAKE_OFFSET(gScreen.CurrentRow, column)] = VGA_MAKE_ENTRY(' ', VGA_MAKE_STYLE(bg, bg));
    }
}

static VOID
_VgaSetCursorPosition(
    _In_ WORD Row,
    _In_ WORD Column
)
{
    gScreen.CurrentRow = Row;
    gScreen.CurrentColumn = Column;

    if (gScreen.CurrentColumn >= VGA_COLUMNS)
    {
        gScreen.CurrentRow++;
        gScreen.CurrentColumn = 0;
    }

    while (gScreen.CurrentRow >= VGA_LINES - 1)
    {
        _VgaScroll();
        gScreen.CurrentRow--;
    }
}


#define VGA_INCREMENT       _VgaSetCursorPosition(gScreen.CurrentRow, gScreen.CurrentColumn + 1)


VOID
VgaInit(
    _In_ PVOID Buffer,
    _In_ VGA_COLOR DefaultForeground,
    _In_ VGA_COLOR DefaultBackground
)
{
    gScreen.Buffer = Buffer;

    gScreen.CurrentColumn = gScreen.CurrentRow = 0;
    gScreen.CurrentColorStyle = VGA_MAKE_STYLE(DefaultForeground, DefaultBackground);

    CLS;
}


VOID
VgaFillScreen(
    _In_ CHAR Ch,
    _In_ VGA_COLOR Foreground,
    _In_ VGA_COLOR Background
)
{
    WORD entry = VGA_MAKE_ENTRY(Ch, VGA_MAKE_STYLE(Foreground, Background));

    for (WORD row = 0; row < VGA_LINES; row++)
    {
        for (WORD column = 0; column < VGA_COLUMNS; column++)
        {
            gScreen.Buffer[VGA_MAKE_OFFSET(row, column)] = entry;
        }
    }
}


VOID
VgaPutChar(
    _In_ CHAR Ch
)
{
    gScreen.Buffer[VGA_MAKE_OFFSET(gScreen.CurrentRow, gScreen.CurrentColumn)] = VGA_MAKE_ENTRY(Ch, gScreen.CurrentColorStyle);
    VGA_INCREMENT;
}


#define CH_NEW_LINE     '\n'
#define CH_TAB          '\t'
#define CH_NULL         '\0'


VOID
VgaPutString(
    _In_ const PCHAR Str
)
{
    SIZE_T index = 0;

    while (Str[index] != CH_NULL)
    {
        switch (Str[index])
        {
        case CH_NEW_LINE:
            _VgaSetCursorPosition(gScreen.CurrentRow + 1, 0);
            break;

        case CH_TAB:
            VgaPutString("    ");
            break;

        default:
            VgaPutChar(Str[index]);
            break;
        }

        index++;
    }
}


VOID
VgaSetForeground(
    _In_ VGA_COLOR Fg
)
{
    // clear the current foreground
    gScreen.CurrentColorStyle &= 0xF0;

    // set the new foreground
    gScreen.CurrentColorStyle |= (Fg & 0x0F);
}


VOID
VgaSetBackground(
    _In_ VGA_COLOR Bg
)
{
    // clear the current background
    gScreen.CurrentColorStyle &= 0x0F;

    // set the new background
    gScreen.CurrentColorStyle |= (Bg << 4);
}