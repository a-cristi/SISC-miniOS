#ifndef _SCREEN_H_
#define _SCREEN_H_

#define MAX_LINES       25
#define MAX_COLUMNS     80
#define MAX_OFFSET      2000    //25 lines * 80 chars


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


#pragma pack(push)
#pragma pack(1)
typedef struct _SCREEN
{
    CHAR    Character;
    BYTE    Color;      // VGA_COLOR
}SCREEN, *PSCREEN;
#pragma pack(pop)


VOID HelloBoot();

VOID 
ScrSetColor(
    VGA_COLOR Color
);

VOID ScrClearScreen();
VOID ScrPutChar(CHAR C, DWORD Pos);
VOID ScrPutString(PCHAR String, DWORD Pos);
VOID ScrPutStringLine(PCHAR String, DWORD Line);


#endif // _SCREEN_H_