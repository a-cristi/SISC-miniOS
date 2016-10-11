#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "defs.h"

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


void HelloBoot();

void SetColor(VGA_COLOR Color);
void ClearScreen();
void PutChar(CHAR C, DWORD Pos);
void PutString(PCHAR String, DWORD Pos);
void PutStringLine(PCHAR String, DWORD Line);


#endif // _SCREEN_H_