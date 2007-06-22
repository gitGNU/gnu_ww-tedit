/*

File: gui_scr.c
Project: W, Layer2 -- WIN32 GUI application
Started: 19th August, 2003
Descrition:
  Win32 GUI application screen API functions.

*/

#include "global.h"
#include "memory.h"

#ifdef _NON_TEXT  /* win32 gui only */

#include "scr.h"

int ScreenHeight = 40;
int ScreenWidth = 80;
int ScreenPosX = -1;
int ScreenPosY = -1;
BOOLEAN bMaximized = FALSE;
char FontName[MAX_FONT_NAME_LEN] = "courier new";
int FontSize = 16;
DWORD Font1Style = 0;
DWORD Font2Style = 0;
DWORD Font3Style = 0;
int scr_height_before_maximize;
int scr_width_before_maximize;

int CurHeight;
int CurWidth;

HWND g_hWnd = NULL;

static int nTop;
static int nLeft;

static HFONT g_hFont = NULL;
SIZE  g_ChSize;

static char * CharBuf = NULL;
static ATTR * AttrBuf = NULL;

static int ScrBufHeight;  /* always >= ScreenHeight */
static int ScrBufWidth;  /* always >= ScreenWidth */

#define SCR_CHAR(y)   (CharBuf + (y)*ScrBufWidth)
#define SCR_ATTR(y)   (AttrBuf + (y)*ScrBufWidth)

int caption_height;
int border_width;

extern void EvQInit ( void );
extern LRESULT CALLBACK WndProc (
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam
    );

void (*disp_handle_resize)(disp_event_t *ev) = NULL;

/* ************************************************************************
   Function: EnableResize
   Description:
     Enables the input of resize events.
*/
void EnableResize(void)
{
}

/* ************************************************************************
   Function: DisableResize
   Description:
     Disables the input of resize events.
*/
void DisableResize(void)
{
}

/* ************************************************************************
   Function: GetScreenMetrix
   Description:
     Initializes ScreenHeight, ScreenWidth and
     CurHeight, CurWidth, hInput and hOutput
*/
void GetScreenMetrix(void)
{
  nTop = nLeft = 0;

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

  return (sizeof(CharInfo) * w * h);
}

/* ************************************************************************
   Function: gettextblock
   Description:
     Gets a text block.
*/
void gettextblock(int x1, int y1, int x2, int y2, void *buf)
{
  CharInfo * pch = (CharInfo *)buf;
  int w = x2 - x1 + 1;

  if (w <= 0 || y2 < y1)
    return;

  for ( ; y1 <= y2; ++y1)
  {
    ATTR * pa;
    char * pc;
    int  c;

    pc = SCR_CHAR(y1) + x1;
    pa = SCR_ATTR(y1) + x1;

    c = w;
    do
    {
      pch->AsciiChar = *pc;
      pch->Attributes = *pa;
    }
    while (++pc, ++pa, ++pch, --c);
  }
}

/* ************************************************************************
   Function: CalcArea
   Description:
     Gets a text block.
*/
static void CalcArea( RECT * prc, int x, int y, int w, int h )
{
  prc->right = (prc->left = x * g_ChSize.cx) + w * g_ChSize.cx;
  prc->bottom = (prc->top = y * g_ChSize.cy) + h * g_ChSize.cy;
}

static void invalidate_rect(RECT *area)
{
  if (area->right == ScreenWidth * g_ChSize.cx)
    area->right += g_ChSize.cx;  /* for the artefact @ maximize */
  if (area->bottom == ScreenHeight * g_ChSize.cy);
    area->bottom += g_ChSize.cy;
  InvalidateRect(g_hWnd, area, FALSE);
}

/* ************************************************************************
   Function: puttextblock
   Description:
     Puts a text block.
*/
void puttextblock(int x1, int y1, int x2, int y2, void *buf)
{
  const CharInfo * pch = (const CharInfo *)buf;
  int w = x2 - x1 + 1;
  RECT area;

  if (w <= 0 || y2 < y1)
    return;

  CalcArea( &area, x1, y1, w, y2 - y1 + 1 );
  invalidate_rect(&area);

  for ( ; y1 <= y2; ++y1)
  {
    ATTR * pa;
    char * pc;
    int  c;

    pc = SCR_CHAR(y1) + x1;
    pa = SCR_ATTR(y1) + x1;

    c = w;
    do
    {
      *pc = pch->AsciiChar;
      *pa = pch->Attributes;
    }
    while (++pc, ++pa, ++pch, --c);
  }
}

/* ************************************************************************
   Function: WriteXY
   Description:
     Displays a string at a specific screen position
*/
void WriteXY(const char *s, int x, int y, BYTE attr)
{
  RECT area;
  int len;

  len = strlen( s );

  if (len == 0)
    return;

  ASSERT(x + len <= ScreenWidth);
  ASSERT(y <= ScreenHeight);

  memcpy( SCR_CHAR(y) + x, s, len );
  memset( SCR_ATTR(y) + x, attr, len );

  CalcArea( &area, x, y, len, 1 );
  invalidate_rect(&area);
}

/* ************************************************************************
   Function: FlexWriteXY
   Description:
     Writes a sting at position x and y. Attributes are alternated
     between attr1 and attr2 when '~' occures
*/
void FlexWriteXY(const char *s, int x, int y, BYTE attr1, BYTE attr2)
{
  RECT area;
  ATTR * pa = SCR_ATTR(y) + x;
  char * pc = SCR_CHAR(y) + x;
  ATTR attr = attr1;
  int  len = 0;

  ASSERT(s != NULL);
  ASSERT(y <= ScreenHeight);

  for ( ; *s; ++s )
  {
    if (*s == '~')
      attr = attr == attr1 ? attr2 : attr1;
    else
    {
      *pa++ = attr;
      *pc++ = *s;
      ++len;
    }
  }

  CalcArea( &area, x, y, len, 1 );
  invalidate_rect(&area);
}

/* ************************************************************************
   Function: FillXY
   Description:
     Fills an area on the screen with a specific character
*/
void FillXY(char c, BYTE attr, int x, int y, int len)
{
  RECT area;

  if (len <= 0)
    return;

  ASSERT(x + len <= ScreenWidth);
  ASSERT(y <= ScreenHeight);

  memset( SCR_CHAR(y) + x, c,    len );
  memset( SCR_ATTR(y) + x, attr, len );

  CalcArea( &area, x, y, len, 1 );
  invalidate_rect(&area);
}

/* ************************************************************************
   Function: HideCursor
   Description:
     Hides the cursor
*/
BOOLEAN bCaretVisible = FALSE;
BOOLEAN bOwnCaret = FALSE;
POINT RealCaretPos = { 0, 0 };
static POINT CaretPos     = { 0, 0 };

static void GShowCaret( BOOL bShow )
{
  if (bShow != bCaretVisible)
  {
    bCaretVisible = bShow;

    if (bOwnCaret)
    {
      if (bShow)
        ShowCaret( g_hWnd );
      else
        HideCaret( g_hWnd );
    }
  }
};

void HideCursor(void)
{
  GShowCaret( FALSE );
}

/* ************************************************************************
   Function: GetCursorParam
   Description:
     Gets information about the cursor's size and visibility state
*/
void GetCursorParam(CursorParam *pInfo)
{
  pInfo->bVisible = bCaretVisible;
  pInfo->Pos = CaretPos;
}

/* ************************************************************************
   Function: RestoreCursor
   Description:
     Restores what GetCursorParam have returned early
*/
void RestoreCursor(CursorParam *pInfo)
{
  GotoXY( pInfo->Pos.x, pInfo->Pos.y );
  GShowCaret( pInfo->bVisible );
}

/* ************************************************************************
   Function: GotoXY
   Description:
     Sets new cursor position on the screen
*/
void GotoXY(int x, int y)
{
  CaretPos.x = x;
  CaretPos.y = y;

  RealCaretPos.x = CaretPos.x * g_ChSize.cx;
  RealCaretPos.y = CaretPos.y * g_ChSize.cy;

  if (bOwnCaret)
    SetCaretPos( RealCaretPos.x, RealCaretPos.y );
}

/* ************************************************************************
   Function: GetCursorXY
   Description:
*/
void GetCursorXY(int *x, int *y)
{
  POINT pos;

  GetCaretPos( &pos );

  *x = pos.x - nLeft;
  *y = pos.y - nTop;
}

/* ************************************************************************
   Function: MapPalette
   Description:
*/
BOOLEAN MapPalette(BYTE *pPalette, int nEntries)
{
  return TRUE;
}

extern HINSTANCE g_hInst;
extern int       g_nCmdShow;

static const TCHAR szWindowClass[] = _T("WW Window Class");
static const TCHAR szTitle[] = _T("WW");
//static const TCHAR szFont[] = _T("Courier");
//static const TCHAR szFont[] = _T("Terminal");
//static const TCHAR szFont[] = _T("lucida console");

static DWORD Colors[16] =
{
  RGB( 0x00, 0x00, 0x00 ),
  RGB( 0x00, 0x00, 0x80 ),
  RGB( 0x00, 0x80, 0x00 ),
  RGB( 0x00, 0x80, 0x80 ),
  RGB( 0x80, 0x00, 0x00 ),
  RGB( 0x80, 0x00, 0x80 ),
  RGB( 0x80, 0x80, 0x00 ),
  RGB( 0xC0, 0xC0, 0xC0 ),

  RGB( 0x80, 0x80, 0x80 ),
  RGB( 0x00, 0x00, 0xFF ),
  RGB( 0x00, 0xFF, 0x00 ),
  RGB( 0x00, 0xFF, 0xFF ),
  RGB( 0xFF, 0x00, 0x00 ),
  RGB( 0xFF, 0x00, 0xFF ),
  RGB( 0xFF, 0xFF, 0x00 ),
  RGB( 0xFF, 0xFF, 0xFF ),
};

/* ************************************************************************
   Function: OnPaint
   Description:
*/
void OnPaint ( HDC hdc, const RECT * upd )
{
  int y;
  int x1, y1, x2, y2;
  ATTR curAttr;
  char pad_buf[2096];
  int w;

  SelectObject( hdc, g_hFont );

  x1 = upd->left / g_ChSize.cx;
  x2 = (upd->right - 1) / g_ChSize.cx + 1;
  if ( x2 > ScreenWidth)
    x2 = ScreenWidth;

  x2 -= x1; // x2 is the width from now on

  if (x2 <= 0)
    return;

  y1 = upd->top / g_ChSize.cy;
  y2 = (upd->bottom - 1) / g_ChSize.cy + 1;
  if (y2 > ScreenHeight)
    y2 = ScreenHeight;

  for ( y = y1; y < y2; ++y )
  {
    int c;
    RECT rc;
    ATTR * pa;
    char * pc;

    rc.left   = x1 * g_ChSize.cx;
    rc.top    = y * g_ChSize.cy;
    rc.bottom = rc.top + g_ChSize.cy;

    pa = SCR_ATTR(y) + x1;
    pc = SCR_CHAR(y) + x1;

    c = x2; // x2 is the width

    do
    {
      int len;

      len = 0;
      curAttr = *pa;

      do
      {
        ++pa;
        ++len;
        --c;
      }
      while (c > 0 && *pa == curAttr);

      rc.right = rc.left + len * g_ChSize.cx;

      SetTextColor( hdc, Colors[curAttr & 15] );
      SetBkColor( hdc, Colors[(curAttr >> 4) & 15] );

      //ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, pc, len, NULL );
      TextOut( hdc, rc.left, rc.top, pc, len );

      /*
      Patch artifacts for the case the window is maximized
      (screen might be not a multiple of g_ChSize)
      */
      if (y == ScreenHeight - 1)  /* did we change the last line on screen? */
      {
        /*
        Output one entire line with the color of the last line
        */
        memset(pad_buf, ' ', sizeof(pad_buf));
        w = min(len, sizeof(pad_buf) - 1);
        if (x2 == ScreenWidth)
          ++w;  /* correct the bottom right corner */
        pad_buf[w] = '\0';
        TextOut( hdc, rc.left, rc.top + g_ChSize.cy, pad_buf, w);
      }

      rc.left = rc.right;
      pc += len;
    }
    while (c > 0);

    /*
    Patch artifacts for the case the window is maximized
    (screen might be not a multiple of g_ChSize)
    */
    if (x2 == ScreenWidth)  /* did we change the last char on screen? */
    {
      /*
      Output one space with the color of the last character
      */
      SetTextColor( hdc, Colors[curAttr & 15] );
      SetBkColor( hdc, Colors[(curAttr >> 4) & 15] );

      /* rc.left is already at the edge of the screen */
      TextOut( hdc, rc.left, rc.top, " ", 1 );
    }
  }
}

//--------------------------------------------------------------------------
// Name         Scr_RegisterClass
//
//
//--------------------------------------------------------------------------
static ATOM Scr_RegisterClass ( void )
{
	WNDCLASS wc;

	wc.style			    = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	  = (WNDPROC)WndProc;
	wc.cbClsExtra		  = 0;
	wc.cbWndExtra		  = 0;
	wc.hInstance		  = g_hInst;
	wc.hIcon			    = NULL;
	wc.hCursor		    = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= NULL; //(HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName	  = NULL;
	wc.lpszClassName	= szWindowClass;

	return RegisterClass(&wc);
}

//--------------------------------------------------------------------------
// Name         Scr_InitInstance
//
//
//--------------------------------------------------------------------------
static BOOL Scr_InitInstance ( void )
{
  RECT rc;
  DWORD style;
  WINDOWPLACEMENT wndpl;
  BOOLEAN is_maximized2;

  is_maximized2 = bMaximized;  /* Destroyed after CreateWindow() */

  style = WS_OVERLAPPED  |
          WS_CAPTION     |
          WS_SYSMENU     |
          WS_SIZEBOX     |
          WS_MAXIMIZEBOX |
          WS_MINIMIZEBOX;

  rc.left = 0;
  rc.top = 0;
  rc.right = ScreenWidth * g_ChSize.cx;
  rc.bottom = ScreenHeight * g_ChSize.cy;

  AdjustWindowRect( &rc, style, FALSE );
  /* Now rc.left/top holds negative values,
  which are the widths of the border and the
  height of the caption bar */
  caption_height = -rc.top;
  border_width = -rc.left;

  g_hWnd = CreateWindow(
              szWindowClass,
              szTitle,
              style,
              CW_USEDEFAULT, 0,
              rc.right - rc.left, rc.bottom - rc.top,
              NULL, NULL, g_hInst,
              NULL
           );

  if (!g_hWnd)
  {
    return FALSE;
  }

  memset(&wndpl, 0, sizeof(wndpl));
  wndpl.length = sizeof(wndpl);
  wndpl.showCmd = SW_SHOW;
  wndpl.rcNormalPosition.left = ScreenPosX;
  wndpl.rcNormalPosition.top = ScreenPosY;
  wndpl.rcNormalPosition.right = ScreenPosX + rc.right + (-rc.left);
  wndpl.rcNormalPosition.bottom = ScreenPosY + rc.bottom + (-rc.top);
  if (!SetWindowPlacement(g_hWnd, &wndpl))
    ;

  if (is_maximized2)
    ShowWindow( g_hWnd, SW_MAXIMIZE );
  else
    ShowWindow( g_hWnd, g_nCmdShow );

  UpdateWindow( g_hWnd );

  return TRUE;
};

/* ************************************************************************
   Function: Scr_ReallocBuf
   Description:
*/
void Scr_ReallocBuf(void)
{
  if (ScrBufHeight < ScreenHeight ||
      ScrBufWidth < ScreenWidth)
  {
    ScrBufHeight = ScreenHeight;
    ScrBufWidth = ScreenWidth;
    s_free(CharBuf);
    s_free(AttrBuf);
    if ( (CharBuf = (char *)alloc( sizeof(char) * (ScrBufWidth) * (ScrBufHeight))) == NULL)
      return /*FALSE*/;
    if ( (AttrBuf = (ATTR *)alloc( sizeof(ATTR) * (ScrBufWidth) * (ScrBufHeight))) == NULL)
      return /*FALSE*/;

    // Erase the "screen"
    memset( CharBuf, ' ', sizeof(char) * ScreenWidth * ScreenHeight );
    memset( AttrBuf, 255, sizeof(ATTR) * ScreenWidth * ScreenHeight );
  }
}

/* ************************************************************************
   Function: OpenConsole
   Description:
*/
BOOLEAN OpenConsole(void)
{
  EvQInit();

  // Create the font
  //
  g_hFont =
    CreateFont(
      FontSize,             //int nHeight,               // height of font
      0,                    //int nWidth,                // average character width
      0,                    //int nEscapement,           // angle of escapement
      0,                    //int nOrientation,          // base-line orientation angle
      400,          //int fnWeight,              // font weight
      FALSE,                //DWORD fdwItalic,           // italic attribute option
      FALSE,                //DWORD fdwUnderline,        // underline attribute option
      FALSE,                //DWORD fdwStrikeOut,        // strikeout attribute option
      DEFAULT_CHARSET,      //DWORD fdwCharSet,          // character set identifier
      OUT_DEFAULT_PRECIS,   //DWORD fdwOutputPrecision,  // output precision
      CLIP_DEFAULT_PRECIS,  //DWORD fdwClipPrecision,    // clipping precision
      DEFAULT_QUALITY,      //DWORD fdwQuality,          // output quality
      FIXED_PITCH | FF_DONTCARE, //DWORD fdwPitchAndFamily,   // pitch and family
      FontName              //LPCTSTR lpszFace           // typeface name
    );
  if (g_hFont == NULL)
    return FALSE;

  // Get character size
  //
  {
    HDC hdc = GetDC( NULL );
    SelectObject( hdc, g_hFont );
    GetTextExtentPoint32( hdc, _T("H"), 1, &g_ChSize );
    ReleaseDC( NULL, hdc );
  }

  ScrBufHeight = max(120, ScreenHeight);
  ScrBufWidth = max(120, ScreenWidth);

  // Init the screen buffer
  //
  if ( (CharBuf = (char *)alloc( sizeof(char) * ScrBufWidth * ScrBufHeight )) == NULL)
    return FALSE;
  if ( (AttrBuf = (ATTR *)alloc( sizeof(ATTR) * ScrBufWidth * ScrBufHeight )) == NULL)
    return FALSE;

  // Erase the "screen"
  memset( CharBuf, ' ', ScreenWidth * ScreenHeight );
  memset( AttrBuf, 0,   sizeof(ATTR) * ScreenWidth * ScreenHeight );

  if (!Scr_RegisterClass())
    return FALSE;
  if (!Scr_InitInstance())
    return FALSE;

  GotoXY( 0, 0 );
  GShowCaret( TRUE );

  return TRUE;
}

/* ************************************************************************
   Function: CloseConsole
   Description:
*/
BOOLEAN CloseConsole(void)
{
  if (g_hWnd)
  {
    DestroyWindow( g_hWnd );
  }
  DeleteObject( g_hFont );
  s_free( CharBuf );
  s_free( AttrBuf );

  return TRUE;
}

/* ************************************************************************
   Function: disp_set_resize_handler
   Description:
*/
void disp_set_resize_handler(void (*handle_resize)(disp_event_t *ev))
{
  disp_handle_resize = handle_resize;
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

  SetWindowText( g_hWnd, buf );
}

#endif  /* _NON_TEXT */

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2003
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
