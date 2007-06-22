/*

File: scr.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Screen and cursor manipulation functions.

*/

#ifndef SCR_H
#define SCR_H

#ifdef WIN32
#ifdef _NON_TEXT
typedef struct
{
  BOOLEAN bVisible;
  POINT   Pos;
} CursorParam;
typedef BYTE ATTR;
typedef struct
{
  char AsciiChar;
  ATTR Attributes;
} CharInfo;
#else  /* _NON_TEXT */
#define CursorParam CONSOLE_CURSOR_INFO
#define CharInfo CHAR_INFO
#endif  /* _NON_TEXT */
#endif  /* WIN32 */

#ifdef MSDOS
#define CursorParam WORD
#define CharInfo WORD
#endif

#ifdef WIN32
#ifdef _NON_TEXT
#define	PutAttr(p, a)  (p)->Attributes = a
#define PutChar(p, c)  (p)->AsciiChar = c
#else
#define	PutAttr(p, a)  p->Attributes = a
#define PutChar(p, c)  p->Char.AsciiChar = c
#endif
#endif

#ifdef MSDOS
#define	PutAttr(p, a)  *(((BYTE *)p) + 1) = a
#define PutChar(p, c)  *((BYTE *)p) = c
#endif

#ifdef UNIX
#ifdef _NON_TEXT
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
typedef struct _CursorParam
{
  int x;
  int y;
  int bVisible;
} CursorParam;
typedef struct _CharInfo
{
  char ch;
  char attr;  /* offset in a palette */
} TCharInfo;
#define CharInfo TCharInfo
extern void _PutChar(TCharInfo *p, char c);
extern void _PutAttr(TCharInfo *p, WORD attr);
#else  /* _NON_TEXT = 0 */
#include <curses.h>
#define CursorParam WORD
#define CharInfo chtype
extern void _PutChar(chtype *p, char c);
extern void _PutAttr(chtype *p, BYTE attr);
#endif  /* _NON_TEXT = 0 */
#define PutAttr(p, a)  _PutAttr(p, a)
#define PutChar(p, a)  _PutChar(p, a)
#endif  /* UNIX */

#include "kbd.h"

extern int CurHeight;
extern int CurWidth;

/* Used only in windows console mode */
extern int nTop;
extern int nLeft;

/* To be stored in wglob.ini */
extern int ScreenHeight;
extern int ScreenWidth;
extern int ScreenPosX;
extern int ScreenPosY;
extern BOOLEAN bMaximized;
#define MAX_FONT_NAME_LEN 256
extern char FontName[MAX_FONT_NAME_LEN];
extern int FontSize;
extern DWORD Font1Style;
extern DWORD Font2Style;
extern DWORD Font3Style;
extern int scr_height_before_maximize;
extern int scr_width_before_maximize;
extern void (*disp_handle_resize)(disp_event_t *ev);

#ifdef UNIX
#ifdef _NON_TEXT
/* TODO: move internal fields into hidden structure inside xscr_i.h */
extern int CharHeight;  /* in pixels */
extern int CharWidth;  /* in pixels */
extern Display *x11_disp;
extern Window WinHndl;
/* to answer Expose events */
void _put_text(int x1, int y1, int x2, int y2);
#endif
#endif

struct disp_ctx;

void EnableResize(void);
void GetScreenMetrix(void);
int CalcRectSz(int w, int h);
void puttextblock(int x1, int y1, int x2, int y2, void *buf);
void gettextblock(int x1, int y1, int x2, int y2, void *buf);
void WriteXY(const char *s, int x, int y, BYTE attr);
void FlexWriteXY(const char *s, int x, int y, BYTE attr1, BYTE attr2);
void FillXY(char c, BYTE attr, int x, int y, int count);
void HideCursor(void);
void GetCursorParam(CursorParam *pInfo);
void RestoreCursor(CursorParam *pInfo);
void GotoXY(int	x, int y);
void GetCursorXY(int *x, int *y);
void SetTitle(const char * sTitle);

BOOLEAN MapPalette(BYTE *pPalette, int nEntries);

BOOLEAN OpenConsole(void);
BOOLEAN CloseConsole(void);

void disp_set_resize_handler(void (*handle_resize)(disp_event_t *ev));
struct disp_ctx *scr_get_disp_ctx(void);

#endif  /* ifndef SCR_H */

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

