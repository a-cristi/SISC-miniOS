#ifndef _SCREEN_H_
#define _SCREEN_H_

#define VGA_MEMORY_BUFFER           (PVOID)(0xB8000ULL)

#define VGA_LINES                   25
#define VGA_COLUMNS                 80
#define MAX_OFFSET                  (VGA_COLUMNS * VGA_LINES)

// Text-mode color constants
typedef enum VGA_COLOR
{
    vgaColorBlack = 0,
    vgaColorBlue,
    vgaColorGreen,
    vgaColorCyan,
    vgaColorRed,
    vgaColorMagenta,
    vgaColorBrown,
    vgaColorLightGrey,
    vgaColorDarkGrey,
    vgaColorLightBlue,
    vgaColorLightGreen,
    vgaColorLightCyan,
    vgaColorLightRed,
    vgaColorLightMagenta,
    vgaColorLightBrown,
    vgaColorWhite,
} VGA_COLOR, *PVGA_COLOR;

VOID
VgaInit(
    _In_ PVOID Buffer,
    _In_ VGA_COLOR DefaultForeground,
    _In_ VGA_COLOR DefaultBackground,
    _In_ BOOLEAN WithHeader
);

VOID
VgaFillScreen(
    _In_ CHAR Ch,
    _In_ VGA_COLOR Foreground,
    _In_ VGA_COLOR Background
);

#define VgaClearScreenToColor(fg, bg)   VgaFillScreen(' ', (fg), (bg))
#define CLS                             VgaClearScreenToColor(vgaColorBlack, vgaColorBlack)

VOID
VgaPutChar(
    _In_ CHAR Ch
);

VOID
VgaPutString(
    _In_ const PCHAR Str
);

VOID
VgaSetForeground(
    _In_ VGA_COLOR Fg
);

VOID
VgaSetBackground(
    _In_ VGA_COLOR Bg
);

VOID
VgaEnableHeader(
    _In_ BOOLEAN Enable
);

#endif // _SCREEN_H_