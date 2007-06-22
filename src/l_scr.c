/*

File: l_scr.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 6th November, 1998
Descrition:
  Console application screen API functions.
  Linux

*/

#include "global.h"
#include <curses.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "scr.h"

/*
Dummy, have no meaning in text console
*/
int ScreenPosX = -1;
int ScreenPosY = -1;
BOOLEAN bMaximized = FALSE;
char FontName[MAX_FONT_NAME_LEN] = "courier new";
int FontSize = 16;
DWORD Font1Style = 0;
DWORD Font2Style = 0;
DWORD Font3Style = 0;

int ScreenHeight;
int ScreenWidth;
int CurHeight;
int CurWidth;

int b_refresh = FALSE;

#if 0
struct termios oldterm;
#endif

/*
Map original PC colors toward CURSES library color constants
*/
#if 0  /* not in use */
BYTE CURSES_TO_PC[8] =
{
  /* CURSES color constants,  corespondent PC colors */
  /* COLOR_BLACK 0   */  0,
  /* COLOR_RED 1     */  4,
  /* COLOR_GREEN 2   */  2,
  /* COLOR_YELLOW 3  */  6,
  /* COLOR_BLUE	4    */  1,
  /* COLOR_MAGENTA 5 */  5,
  /* COLOR_CYAN	6    */  3,
  /* COLOR_WHITE 7   */  7
};
#endif

/* Use PC color as index to get CURSES color constant */
static BYTE PC_TO_CURSES[8] =
{
  COLOR_BLACK,
  COLOR_BLUE,
  COLOR_GREEN,
  COLOR_CYAN,
  COLOR_RED,
  COLOR_MAGENTA,
  COLOR_YELLOW,
  COLOR_WHITE
};

/*
Use a byte as color/background combination from a
PC palette to get the CURSES color/backgraound color pair index.
Notice that not all the entries in the array will
have their CURSES pair counterpart. As CURSES can
have up to 64 color pair definitions at one and the same time.
*/
static BYTE PaletteToPairs[256];
static BOOLEAN SetBold[256];  /* When the color should be set using A_BOLD */
static BOOLEAN PaletteDefined[256];  /* Which color is defined */

/* ************************************************************************
   Function: StartCurses
   Description:
*/
static BOOLEAN StartCurses(void)
{
  //struct termios term;
  extern void GetNCursesKeys(void);

  if (initscr() == NULL)
  {
    TRACE0("error: initializing CURSES library failed.\n");
    return FALSE;
  }

  if (tigetstr("cup") == NULL)
  {
    TRACE0("error: this terminal doesn't support cursor oriented operations.\n");
    endwin();
    return FALSE;
  }

  if (start_color() == ERR)
  {
    TRACE0("error: start_color() failed.\n");
    endwin();
    return FALSE;
  }

  /*
  IMPORTANT: Don't uncomment tcget/setattr()! It's an example of a great
  problem which took 2-3 years to diagnose properly and fix -- when
  under ncurses don' use direct file i/o to handle the keyboard.
  */
  #if 0
  if (tcgetattr(fileno(stdout), &oldterm) != 0)
  {
    TRACE0("error: tcgetattr() failed.\n");
    endwin();
    return FALSE;
  }

  memcpy(&term, &oldterm, sizeof(struct termios));

  /* Disable INTR, QUIT and SUSP characters to be processed by the terminal */
  term.c_lflag &= ~ISIG;
  /* Disable start/stop on output, then START and STOP
  can be read as ordinary characters */
  term.c_iflag &= ~IXON;
  term.c_lflag &= ~ICANON;
  term.c_lflag &= ~ECHO;
  #endif
  #if 0  /* this prevents the editor from working in rxvt */
  term.c_cc[VMIN] = 0;
  term.c_cc[VTIME] = 0;
  #endif
  #if 0
  if (tcsetattr(fileno(stdout), TCSANOW, &term) != 0)
  {
    TRACE0("error: tcsetattr() failed.\n");
    endwin();
    return FALSE;
  }

  if (fcntl(fileno(stdin), F_SETFL,  O_NONBLOCK) == -1)
  {
    TRACE0("error: fcntl() failed.\n");
    endwin();
    return FALSE;
  }
  #endif

  /*
  IMPORTANT:
  Don't uncomment the function calls below. They show that
  some of the settings of ncurses must not be invoked.
  */
  //immedok(curscr, TRUE);
  //cbreak();
  //typeahead(-1);
  raw();  /* no interrupt, quist, suspend and flow control */
  noecho();  /* no auto echo */
  nonl();  /* don't wait for new line to process keys */
  nodelay(stdscr, TRUE);
  intrflush(stdscr, FALSE);

  GetNCursesKeys();

  return TRUE;
}

/* ************************************************************************
   Function: StopCurses
   Description:
*/
BOOLEAN StopCurses(void)
{
  //tcsetattr(fileno(stdout), TCSANOW, &oldterm);

  if (endwin() == ERR)
  {
    TRACE0("error: CURSES library failed to restore the original screen.\n");
    return FALSE;
  }
  return TRUE;
}

/* ************************************************************************
   Function: MapPalette
   Description:
     Generates PaletteToPairs[] and the actual pairs for all
     the entries in pPalette.
   Returns:
     TRUE sucessfully mapped all the colors from pPalette.
     FALSE no enough pairs to map all the colors from pPalette.
*/
BOOLEAN MapPalette(BYTE *pPalette, int nEntries)
{
  int i;
  int nPairIndex;
  BYTE nColor;
  BYTE nBackground;
  BOOLEAN bSetBold;
  int r;

  memset(SetBold, 0, 256);  /* Set to FALSE all the entries */
  memset(PaletteDefined, 0, 256);  /* Still no entries defined */

  /*
  TRACE1("COLOR_PAIRS: %d\n", COLOR_PAIRS);
  TRACE1("entries: %d\n", nEntries);
  */
  nPairIndex = 1;  /* 0 is reserved */
  for (i = 0; i < nEntries; ++i)
  {
    if (PaletteDefined[pPalette[i]])
      continue;

    if (nPairIndex == COLOR_PAIRS)
    {
      TRACE1("no enough color pairs (max %d)\n", COLOR_PAIRS);
      return FALSE;  /* No enougn color pairs to define a whole palette */
    }

    /* TRACE1("dissolve: %#x\n", pPalette[i]); */
    nColor = pPalette[i] & 0x0f;
    nBackground = pPalette[i] >> 4;
    bSetBold = FALSE;
    if (nColor >= 8)
    {
      nColor -= 8;
      bSetBold = TRUE;
    }
    /*
    TRACE2("i: %d, nPairIndex: %d, ", i, nPairIndex);
    TRACE2("nColor: %#x, nBackground: %#x\n", nColor, nBackground);
    TRACE2("pair i1: %x, i2: %x\n", PC_TO_CURSES[nColor], PC_TO_CURSES[nBackground]);
    */
    r = init_pair(nPairIndex, PC_TO_CURSES[nColor], PC_TO_CURSES[nBackground]);
    ASSERT(r != ERR);
    PaletteToPairs[pPalette[i]] = nPairIndex;
    SetBold[pPalette[i]] = bSetBold;
    PaletteDefined[pPalette[i]] = TRUE;
    ++nPairIndex;
  }

  return TRUE;
}

/* ************************************************************************
   Function: CalcRectSz
   Description:
*/
int CalcRectSz(int w, int h)
{
  ASSERT(w > 0);
  ASSERT(h > 0);

  /* +1 for the trailing zero, as NCURSES works with null terminated
  strings of chtype */
  return (w * h + 1) * sizeof(chtype);
}

/* ************************************************************************
   Function: puttextblock
   Description:
   Note: x2, y2 are inclusive coordinates!
*/
void puttextblock(int x1, int y1, int x2, int y2, void *buf)
{
  int i;
  int nWidth;

  ASSERT(x1 <= x2);
  ASSERT(y1 <= y2);
  ASSERT(buf != NULL);

  /*TRACE2("puttext: x2=%d, y2=%d\n", x2, y2);*/
  nWidth = x2 - x1 + 1;
  for (i = y1; i <= y2; ++i)
  {
    VERIFY(mvaddchnstr(i, x1, buf, nWidth) != ERR);
    buf = (chtype *)buf + nWidth;
  }
  /*VERIFY(refresh() != ERR);*/ /* too much refreshing is bad for ncurses! */
  b_refresh = TRUE;
}

/* ************************************************************************
   Function: gettextblock
   Description:
   Note: x2, y2 are inclusive coordinates!
*/
void gettextblock(int x1, int y1, int x2, int y2, void *buf)
{
  int i;
  int nWidth;

  ASSERT(x1 <= x2);
  ASSERT(y1 <= y2);
  ASSERT(buf != NULL);

  nWidth = x2 - x1 + 1;
  for (i = y1; i <= y2; ++i)
  {
    VERIFY(mvinchnstr(i, x1, buf, nWidth) != ERR);
    buf = (chtype *)buf + nWidth;
  }
}

/* ************************************************************************
   Function: WriteXY
   Description:
     Displays a string at a specific screen position
*/
void WriteXY(const char *s, int x, int y, BYTE attr)
{
  chtype buf[255];
  const char *p;
  chtype *b;

  ASSERT(s != NULL);
  ASSERT(x + strlen(s) <= ScreenWidth);
  ASSERT(y <= ScreenHeight);
  ASSERT(ScreenWidth < _countof(buf));

  for (p = s, b = buf; *p; ++p)
  {
    *b = *p;
    ASSERT(PaletteDefined[attr]);
    *b |= COLOR_PAIR(PaletteToPairs[attr]);
    if (SetBold[attr])
      *b |= A_BOLD;
    ++b;
  }

  puttextblock(x, y, (int)(x + (p - s - 1)), y, buf);
}

/* ************************************************************************
   Function: PutChar
   Description:
*/
void _PutChar(chtype *p, char c)
{
  *p &= 0xffffff00;
  *p |= c;
}

/* ************************************************************************
   Function: PutAttr
   Description:
*/
void _PutAttr(chtype *p, BYTE attr)
{
  *p &= 0xff;
  ASSERT(PaletteDefined[attr]);
  *p |= COLOR_PAIR(PaletteToPairs[attr]);
  if (SetBold[attr])
    *p |= A_BOLD;
}

/* ************************************************************************
   Function: FlexWriteXY
   Description:
*/
void FlexWriteXY(const char *s, int x, int y, BYTE attr1, BYTE attr2)
{
  const char *c;
  chtype *d;
  chtype buf[255];
  BYTE attr;
  int ctilds = 0;

  ASSERT(ScreenWidth < _countof(buf));
  ASSERT(s != NULL);
  ASSERT(y <= ScreenHeight);

  attr = attr1;
  d = buf;
  for (c = s; *c; ++c)
  {
    if (*c == '~')
    {
      attr = (attr == attr1) ? attr2 : attr1;
      ctilds++;
      continue;
    }
    *d = *c;
    ASSERT(PaletteDefined[attr]);
    *d |= COLOR_PAIR(PaletteToPairs[attr]);
    if (SetBold[attr])
      *d |= A_BOLD;
    ++d;
  }

  puttextblock(x, y, (int)(x + (c - (char *)s) - 1 - ctilds), y, buf);
}

/* ************************************************************************
   Function: FillXY
   Description:
     Fills an area on the screen with a specific character
*/
void FillXY(char c, BYTE attr, int x, int y, int count)
{
  int i;
  chtype buf[255];

  ASSERT(count > 0);
  ASSERT(x + count <= ScreenWidth);
  ASSERT(y <= ScreenHeight);
  ASSERT(ScreenWidth < _countof(buf));

  for (i = 0; i < count; ++i)
  {
    buf[i] = c;
    ASSERT(PaletteDefined[attr]);
    buf[i] |= COLOR_PAIR(PaletteToPairs[attr]);
    if (SetBold[attr])
      buf[i] |= A_BOLD;
  }

  puttextblock(x, y, x + count - 1, y, buf);
}

/* ************************************************************************
   Function: HideCursor
   Description:
*/
void HideCursor(void)
{
  curs_set(0);
}

/* ************************************************************************
   Function: GetCursorParam
   Description:
*/
void GetCursorParam(CursorParam *pInfo)
{
  ASSERT(pInfo != NULL);

  *pInfo = curs_set(1);
}

/* ************************************************************************
   Function: RestoreCursor
   Description:
*/
void RestoreCursor(CursorParam *pInfo)
{
  ASSERT(pInfo != NULL);

  curs_set(*pInfo);
}

/* ************************************************************************
   Function: GotoXY
   Description:
*/
void GotoXY(int	x, int y)
{
  VERIFY(move(y, x) != ERR);
  VERIFY(refresh() != ERR);
}

/* ************************************************************************
   Function: GetCursorXY
   Description:
*/
void GetCursorXY(int *x, int *y)
{
  getyx(curscr, *y, *x);
}

/* ************************************************************************
   Function: GetCursorXY
   Description:
*/
void GetScreenMetrix(void)
{
  getmaxyx(stdscr, ScreenHeight, ScreenWidth);
  CurHeight = ScreenHeight - 1;
  CurWidth = ScreenWidth - 1;
}

/* ************************************************************************
   Function: OpenConsole
   Description:
*/
BOOLEAN OpenConsole(void)
{
  return StartCurses();
}

/* ************************************************************************
   Function: disp_set_resize_handler
   Description:
     Resize in not a modal event in the console application
*/
void disp_set_resize_handler(void (*handle_resize)(disp_event_t *ev))
{
}

/* ************************************************************************
   Function: OpenConsole
   Description:
*/
BOOLEAN CloseConsole(void)
{
  return StopCurses();
}

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
   Function: SetTitle
   Description: Sets the window title.
*/
void SetTitle ( const char * sTitle )
{
}

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2005
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

