/*

File: w_scr.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 6th November, 1998
Descrition:
  Win32 Console application screen API functions.

*/

#include "global.h"

#ifndef _NON_TEXT  /* text mode console only */

#include "scr.h"

int ScreenHeight;
int ScreenWidth;

/* Those are dummy, have no meaning in console mode */
int ScreenPosX = -1;
int ScreenPosY = -1;
BOOLEAN bMaximized = FALSE;
char FontName[MAX_FONT_NAME_LEN] = "courier new";
int FontSize = 16;
DWORD Font1Style = 0;
DWORD Font2Style = 0;
DWORD Font3Style = 0;

int CurHeight;
int CurWidth;

HANDLE hInput;
HANDLE hOutput;

int nTop;
int nLeft;

/* ************************************************************************
   Function: EnableResize
   Description:
     Enables the input of resize events.
*/
void EnableResize(void)
{
  DWORD ConsoleMode;

  GetConsoleMode(hInput, &ConsoleMode);
  ConsoleMode |= ENABLE_WINDOW_INPUT;
  SetConsoleMode(hInput, ConsoleMode);
}

/* ************************************************************************
   Function: DisableResize
   Description:
     Disables the input of resize events.
*/
void DisableResize(void)
{
  DWORD ConsoleMode;

  GetConsoleMode(hInput, &ConsoleMode);
  ConsoleMode &= ~ENABLE_WINDOW_INPUT;
  SetConsoleMode(hInput, ConsoleMode);
}

/* ************************************************************************
   Function: GetScreenMetrix
   Description:
     Initializes ScreenHeight, ScreenWidth and
     CurHeight, CurWidth, hInput and hOutput
*/
void GetScreenMetrix(void)
{
  CONSOLE_SCREEN_BUFFER_INFO BufInfo;
  DWORD ConsoleMode;

  hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  hInput = GetStdHandle(STD_INPUT_HANDLE);

  ASSERT(hOutput != INVALID_HANDLE_VALUE);
  ASSERT(hInput != INVALID_HANDLE_VALUE);

  /* Input flags */
  GetConsoleMode(hInput, &ConsoleMode);
  ConsoleMode = 0;
  ConsoleMode |= ENABLE_MOUSE_INPUT;
  SetConsoleMode(hInput, ConsoleMode);

  GetConsoleScreenBufferInfo(hOutput, &BufInfo);
  nTop = BufInfo.srWindow.Top;
  nLeft = BufInfo.srWindow.Left;
  ScreenWidth = BufInfo.srWindow.Right - BufInfo.srWindow.Left + 1;
  ScreenHeight = BufInfo.srWindow.Bottom - BufInfo.srWindow.Top + 1;
  CurWidth = ScreenWidth - 1;
  CurHeight = ScreenHeight - 1;
}

/* ************************************************************************
   Function: CalcRectSz
   Description:
     Calculates the size of a region to be used in gettextblock or
     puttextblock
*/
int CalcRectSz(int w, int h)
{
  ASSERT((signed)w > 0);
  ASSERT((signed)h > 0);
  ASSERT(h <= ScreenHeight);

  return (sizeof(CHAR_INFO) * w * h);
}

/* ************************************************************************
   Function: puttextblock
   Description:
     Puts a text block.
*/
void puttextblock(int x1, int y1, int x2, int y2, void *buf)
{
  COORD BufSize;
  COORD BufCoord;
  SMALL_RECT region;

  BufSize.X = x2 - x1 + 1;
  BufSize.Y = y2 - y1 + 1;
  BufCoord.X = 0;
  BufCoord.Y = 0;
  region.Left = x1 + nLeft;
  region.Top = y1 + nTop;
  region.Right = x2 + nLeft;
  region.Bottom = y2 + nTop;
  
  WriteConsoleOutput(hOutput, buf, BufSize, BufCoord, &region);
}

/* ************************************************************************
   Function: gettextblock
   Description:
     Gets a text block.
*/
void gettextblock(int x1, int y1, int x2, int y2, void *buf)
{
  COORD BufSize;
  COORD BufCoord;
  SMALL_RECT region;

  BufSize.X = x2 - x1 + 1;
  BufSize.Y = y2 - y1 + 1;
  BufCoord.X = 0;
  BufCoord.Y = 0;
  region.Left = x1 + nLeft;
  region.Top = y1 + nTop;
  region.Right = x2 + nLeft;
  region.Bottom = y2 + nTop;

  ReadConsoleOutput(hOutput, buf, BufSize, BufCoord, &region);
}

/* ************************************************************************
   Function: WriteXY
   Description:
     Displays a string at a specific screen position
*/
void WriteXY(const char *s, int x, int y, BYTE attr)
{
  CHAR_INFO buf[255];
  const char *p;
  CHAR_INFO *b;

  ASSERT(s != NULL);
  ASSERT(x + strlen(s) <= (unsigned)ScreenWidth);
  ASSERT(y <= ScreenHeight);

  for (p = s, b = buf; *p; ++p, ++b)
  {
    b->Char.AsciiChar = *p;
    b->Attributes = attr;
  }

  puttextblock(x, y, x + (p - s - 1), y, buf);
}

/* ************************************************************************
   Function: FlexWriteXY
   Description:
     Writes a sting at position x and y. Attributes are alternated
     between attr1 and attr2 when '~' occures
*/
void FlexWriteXY(const char *s, int x, int y, BYTE attr1, BYTE attr2)
{
  char *c;
  CHAR_INFO *d;
  CHAR_INFO buf[255];
  BYTE attr;
  int ctilds = 0;

  ASSERT(s != NULL);
  ASSERT(y <= ScreenHeight);

  attr = attr1;
  d = buf;
  for (c = (char *)s; *c; ++c)
  {
    if (*c == '~')
    {
      attr = (attr == attr1) ? attr2 : attr1;
      ++ctilds;
      continue;
    }
    d->Char.AsciiChar = *c;
    d->Attributes = attr;
    ++d;
  }

  puttextblock(x, y, x + (c - s) - 1 - ctilds, y, buf);
}

/* ************************************************************************
   Function: FillXY
   Description:
     Fills an area on the screen with a specific character
*/
void FillXY(char c, BYTE attr, int x, int y, int count)
{
  int i;
  CHAR_INFO buf[255];

  ASSERT(count > 0);
  ASSERT(x + count <= ScreenWidth);
  ASSERT(y <= ScreenHeight);

  for (i = 0; i < count; ++i)
  {
    buf[i].Char.AsciiChar = c;
    buf[i].Attributes = attr;
  }

  puttextblock(x, y, x + count - 1, y, buf);
}

/* ************************************************************************
   Function: HideCursor
   Description:
     Hides the cursor
*/
void HideCursor(void)
{
  CONSOLE_CURSOR_INFO Info;

  GetConsoleCursorInfo(hOutput, &Info);
  Info.bVisible = FALSE;
  SetConsoleCursorInfo(hOutput, &Info);
}

/* ************************************************************************
   Function: GetCursorParam
   Description:
     Gets information about the cursor's size and visibility state
*/
void GetCursorParam(CursorParam *pInfo)
{
  GetConsoleCursorInfo(hOutput, pInfo);
}

/* ************************************************************************
   Function: RestoreCursor
   Description:
     Restores what GetCursorParam have returned early
*/
void RestoreCursor(CursorParam *pInfo)
{
  SetConsoleCursorInfo(hOutput, pInfo);
}

/* ************************************************************************
   Function: GotoXY
   Description:
     Sets new cursor position on the screen
*/
void GotoXY(int x, int y)
{
  COORD c;

  c.X = x + nLeft;
  c.Y = y + nTop;
  SetConsoleCursorPosition(hOutput, c);
}

/* ************************************************************************
   Function: GetCursorXY
   Description:
*/
void GetCursorXY(int *x, int *y)
{
  CONSOLE_SCREEN_BUFFER_INFO c;

  GetConsoleScreenBufferInfo(hOutput, &c);
  *x = c.dwCursorPosition.X - nLeft;
  *y = c.dwCursorPosition.Y - nTop;
}

/* ************************************************************************
   Function: MapPalette
   Description:
*/
BOOLEAN MapPalette(BYTE *pPalette, int nEntries)
{
  return TRUE;
}

/* ************************************************************************
   Function: OpenConsole
   Description:
*/
BOOLEAN OpenConsole(void)
{
  return TRUE;
}

/* ************************************************************************
   Function: disp_set_resize_handler
   Description:
     There are no resize events generated in the Windows (WinXP)
     text console.
*/
void disp_set_resize_handler(void (*handle_resize)(struct event *))
{
}

/* ************************************************************************
   Function: CloseConsole
   Description:
*/
BOOLEAN CloseConsole(void)
{
  return TRUE;
}


/* ************************************************************************
   Function: SetTitle
   Description: Sets the window title.
*/
void SetTitle ( const char * sTitle )
{
  static const char sProg[] = " - WW";
  char * buf;

  if (buf = (char *)_alloca( strlen( sTitle ) + sizeof(sProg) ))
  {
  	strcpy(buf, sTitle);
  	strcat(buf, sProg);
  }

  SetConsoleTitle( sTitle );
}

#endif

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2002
Petar Marinov

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

