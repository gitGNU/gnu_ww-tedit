/*

File: scr.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Screen and cursor manipulation functions.
  DOS

*/

#include "global.h"
#include "scr.h"
#include <conio.h>

#include <dpmi.h>

int ScreenHeight;
int ScreenWidth;
int CurHeight;
int CurWidth;

/* ************************************************************************
   Function: GetScreenMetrix
   Description:
     Initializes ScreenHeight, ScreenWidth and
     CurHeight, CurWidth
*/
void GetScreenMetrix(void)
{
  struct text_info r;

  gettextinfo(&r);
  ScreenHeight = r.screenheight;
  ScreenWidth = r.screenwidth;
  CurHeight = ScreenHeight - 1;
  CurWidth = ScreenWidth - 1;
}

/* ************************************************************************
   Function: CalcRectSz
   Description:
*/
int CalcRectSz(int w, int h)
{
  ASSERT(w > 0);
  ASSERT(h > 0);

  return (w * h * 2);
}

/* ************************************************************************
   Function: puttextblock
   Description:
     Puts a text block. Adjust the coordinates to be 1 relative.
*/
void puttextblock(int x1, int y1, int x2, int y2, void *buf)
{
  puttext(x1 + 1, y1 + 1, x2 + 1, y2 + 1, buf);
}

/* ************************************************************************
   Function: gettextblock
   Description:
     Gets a text block. Adjust the coordinates to be 0 relative.
*/
void gettextblock(int x1, int y1, int x2, int y2, void *buf)
{
  gettext(x1 + 1, y1 + 1, x2 + 1, y2 + 1, buf);
}

/* ************************************************************************
   Function: WriteXY
   Description:
     Displays a string at a specific screen position
*/
void WriteXY(const char *s, int x, int y, BYTE attr)
{
  BYTE buf[255 * 2];
  const char *p;
  BYTE *b;

  ASSERT(s != NULL);
  ASSERT(x + strlen(s) <= ScreenWidth);
  ASSERT(y <= ScreenHeight);

  for (p = s, b = buf; *p; ++p)
  {
    *b++ = *p;
    *b++ = attr;
  }

  puttextblock((int)x, (int)y, (int)(x + (p - s - 1)), (int)y, buf);
}

/* ************************************************************************
   Function: FlexWriteXY
   Description:
*/
void FlexWriteXY(const char *s, int x, int y, BYTE attr1, BYTE attr2)
{
  char *c;
  char *d;
  BYTE buf[255 * 2];
  BYTE attr;
  int ctilds = 0;

  ASSERT(s != NULL);
  ASSERT(y <= ScreenHeight);

  attr = attr1;
  d = (char *)buf;
  for (c = (char *)s; *c; ++c)
  {
    if (*c == '~')
    {
      attr = (attr == attr1) ? attr2 : attr1;
      ctilds++;
      continue;
    }
    *d++ = *c;
    *d++ = attr;
  }

  puttextblock((int)x, (int)y, (int)(x + (c - (char *)s) - 1 - ctilds), (int)y, buf);
}

/* ************************************************************************
   Function: FillXY
   Description:
     Fills an area on the screen with a specific character
*/
void FillXY(char c, BYTE attr, int x, int y, int count)
{
  int i;
  BYTE buf[255 * 2];

  ASSERT(count > 0);
  ASSERT(x + count <= ScreenWidth);
  ASSERT(y <= ScreenHeight);

  for (i = 0; i < count; ++i)
  {
    buf[i * 2] = c;
    buf[i * 2 + 1] = attr;
  }

  puttextblock(x, y, x + count - 1, y, buf);
}

/* ************************************************************************
   Function: HideCursor
   Description:
*/
void HideCursor(void)
{
  #ifdef __TURBOC__
  union REGS inregs, outregs;

  inregs.h.ah = 0x1;
  inregs.x.cx = 0x2000;
  int86(0x10, &inregs, &outregs);
  #else  /* To be compiled with gcc */
  __dpmi_regs r;

  r.h.ah = 0x1;
  r.x.cx = 0x2000;
  __dpmi_int(0x10, &r);
  #endif
}

/* ************************************************************************
   Function: GetCursorParam
   Description:
*/
void GetCursorParam(CursorParam *pInfo)
{
  #ifdef __TURBOC__
  union REGS inregs, outregs;

  inregs.h.ah = 0x3;
  inregs.h.bh = 0;
  int86(0x10, &inregs, &outregs);
  *pInfo = outregs.x.cx;
  #else  /* To be compiled with gcc */
  __dpmi_regs r;

  r.h.ah = 0x3;
  r.h.bh = 0;
  __dpmi_int(0x10, &r);
  *pInfo = r.x.cx;
  #endif
}

/* ************************************************************************
   Function: RestoreCursor
   Description:
*/
void RestoreCursor(CursorParam *pInfo)
{
  #ifdef __TURBOC__
  union REGS inregs, outregs;

  inregs.h.ah = 0x1;
  inregs.x.cx = *pInfo;
  int86(0x10, &inregs, &outregs);
  #else  /* To be compiled with gcc */
  __dpmi_regs r;

  r.h.ah = 0x1;
  r.x.cx = *pInfo;
  __dpmi_int(0x10, &r);
  #endif
}

/* ************************************************************************
   Function: GotoXY
   Description:
*/
void GotoXY(int	x, int y)
{
  gotoxy(x + 1, y + 1);
}

/* ************************************************************************
   Function: GetCursorXY
   Description:
*/
void GetCursorXY(int *x, int *y)
{
  struct text_info r;

  gettextinfo(&r);
  *x = r.curx - 1;
  *y = r.cury - 1;
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
}
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

