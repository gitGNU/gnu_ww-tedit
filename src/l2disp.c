/*

File: l2disp.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Layer 2 display support: Displaying current file, current position, status
  line, status line messages, user-screen (AltF5).

*/

#include "global.h"

#include "l2disp.h"
#include "memory.h"
#include "disp.h"
#include "nav.h"
#include "wline.h"
#include "palette.h"
#include "l1def.h"
#include "defs.h"
#include "path.h"
#include "wrkspace.h"

static BYTE *pUserScreenBuf = NULL;
static disp_cursor_param_t UserCursor;

/* ************************************************************************
   Function: PutText
   Description:
     Call-back function to display a line. DJGPP and Win32 console
     application release.
*/
static int PutText(int nStartX, int nStartY, int nWidth, int nYLine,
  int nPaletteStart, const TLineOutput *pBuf, dispc_t *disp)
{
  unsigned char *OutputBuf;
  int output_buf_size;
  const TLineOutput *b;
  void *x;
  int i;
  int rect_size;
  int screen_w;

  /* allocate some safe output buffer in the stack */
  output_buf_size = MAX_WIN_WIDTH * 3 * sizeof(int);
  OutputBuf = alloca(MAX_WIN_WIDTH * 3 * sizeof(int));

  rect_size = disp_calc_rect_size(disp, nWidth, 1);
  x = OutputBuf;

  /* if the buffer in the stack is too small then allocate
  a buffer in the heap */
  if (rect_size > output_buf_size)
    x = s_alloc(rect_size);
  disp_cbuf_reset(disp, x, rect_size);  /* init char buf */

  b = pBuf;
  screen_w = disp_wnd_get_width(disp);

  for (i = 0; i < screen_w; ++i, ++b)
    disp_cbuf_put_char_attr(disp, x, i, b->c, GetColor(b->t + nPaletteStart));

  disp_put_block(disp, nStartX, nStartY + nYLine, nWidth, 1, x);

  disp_cbuf_mark_invalid(disp, x);  /* only for extra safety */
  if (x != OutputBuf)
    s_free(x);

  return (0);
}

/* ************************************************************************
   Function: ConsoleUpdatePage
   Description:
     Updates current page at the screen. DJGPP and Win32 console
     application wrap of UpdatePage().
*/
void ConsoleUpdatePage(TFile *pFile, int x1, int y1, int x2, int y2,
  int nPaletteStart, void *pExtraColorInterf, dispc_t *disp)
{
  ASSERT(VALID_PFILE(pFile));

  UpdatePage(pFile, x1, y1, x2 - x1, y2 - y1, nPaletteStart,
    &stSearchContext, PutText, pExtraColorInterf, disp);
}

/* ************************************************************************
   Function: DisplayStatusStr
   Description:
     Displays a string at the status line.
     bClean = TRUE to clear the line before displaying the string.
     bShowCursor = TRUE to set the cursor at the end of the string.
*/
void DisplayStatusStr(const char *sStatus, BYTE attr, BOOLEAN bClean,
  BOOLEAN bShowCursor, dispc_t *disp)
{
  int cur_height;

  ASSERT(sStatus != NULL);
  ASSERT(sStatus[0] != '\0');  /* Check for valid string -- not empty */
  ASSERT(strlen(sStatus) < MAX_WIN_WIDTH);  /* And with an end marker */

  cur_height = disp_wnd_get_height(disp) - 1;
  if (bClean)
    disp_fill(disp, ' ', GetColor(attr), 0, cur_height, disp_wnd_get_width(disp));

  disp_write(disp, sStatus, 0, cur_height, GetColor(attr));

  if (bShowCursor)
    disp_cursor_goto_xy(disp, strlen(sStatus), cur_height);
}

/* ************************************************************************
   Function: DisplayStatusStr2
   Description:
     Displays status string with flex attribute.
     Example: '~T~est' will highlight 'T'.
*/
void DisplayStatusStr2(const char *sStatus, BYTE attr1, BYTE attr2, dispc_t *disp)
{
  int cur_height;

  ASSERT(sStatus != NULL);
  ASSERT(sStatus[0] != '\0');  /* Check for valid string -- not empty */
  ASSERT(strlen(sStatus) < MAX_WIN_WIDTH);  /* And with an end marker */

  cur_height = disp_wnd_get_height(disp) - 1;
  disp_fill(disp, ' ', GetColor(attr1), 0, cur_height, disp_wnd_get_width(disp));

  disp_flex_write(disp, sStatus, 0, cur_height, GetColor(attr1), GetColor(attr2));
}

/* ************************************************************************
   Function: SaveStatusLine
   Description:
     Saves the status line in the heap.
     Returns a pointer to the image.
*/
disp_char_buf_t *SaveStatusLine(dispc_t *disp)
{
  disp_char_buf_t *statln_cbuf;
  int width;
  int cur_height;
  int rect_size;

  cur_height = disp_wnd_get_height(disp) - 1;
  width = disp_wnd_get_width(disp);
  rect_size = disp_calc_rect_size(disp, width, 1);

  statln_cbuf = s_alloc(rect_size);
  disp_cbuf_reset(disp, statln_cbuf, rect_size);  /* init statln_cbuf */

  disp_get_block(disp, 0, cur_height, width, 1, statln_cbuf);

  return statln_cbuf;
}

/* ************************************************************************
   Function: RestoreStatusLine
   Description:
     Restores a status line image from the heap.
*/
void RestoreStatusLine(disp_char_buf_t *statln_cbuf, dispc_t *disp)
{
  int width;
  int cur_height;

  cur_height = disp_wnd_get_height(disp) - 1;
  width = disp_wnd_get_width(disp);

  disp_put_block(disp, 0, cur_height, width, 1, statln_cbuf);
  disp_cbuf_mark_invalid(disp, statln_cbuf);  /* only for extra safety */
  s_free(statln_cbuf);
}

/* ************************************************************************
   Function: ConsoleUpdateStatusLine
   Description:
     Updates status line.
   Returns:
     0 - status line was not updated.
*/
int ConsoleUpdateStatusLine(TFile *pFile, dispc_t *disp)
{
  unsigned char OutputBuf[MAX_WIN_WIDTH * 3 * sizeof(int)];  /* provisional */
  void *x;
  char sStatus[255];
  char sChanged[2];
  char *pStatLn;
  char *pChangedPos;
  BYTE c;
  char cSep;
  int width;
  int cur_height;
  int rect_size;
  int i;

  if (!pFile->bUpdateStatus)
    return 0;

  cur_height = disp_wnd_get_height(disp) - 1;
  width = disp_wnd_get_width(disp);
  rect_size = disp_calc_rect_size(disp, width, 1);

  x = OutputBuf;
  if (rect_size > sizeof(OutputBuf))
    x = s_alloc(rect_size);
  disp_cbuf_reset(disp, x, rect_size);  /* init char buf */

  pFile->bUpdateStatus = FALSE;

  if (pFile->sMsg[0] != '\0')  /* Display the message only */
  {
    memset(sStatus, ' ', sizeof(sStatus));
    strcpy(sStatus, pFile->sMsg);
    *strchr(sStatus, '\0') = ' ';
    sStatus[width] = '\0';
    DisplayStatusStr(sStatus, coStatus, FALSE, FALSE, disp);
    goto _exit;
  }

  /*
  Compose the line in WrtLnBuf and use puttext() to display the
  status line at the end of the function.
  */
  sChanged[0] = '\0';
  if (pFile->bChanged && pFile->bDisplayChanged)
  {
    sChanged[0] = '*';
    sChanged[1] = '\0';
  }

  memset(sStatus, ' ', sizeof(sStatus));
  #if USE_ASCII_BOXES
  cSep = '|';
  #else
  cSep = '³';
  #endif
  sprintf(sStatus, " %d:%d %c %s%s", pFile->nCol + 1, pFile->nRow + 1,
    cSep, sChanged, pFile->sTitle);
  *strchr(sStatus, '\0') = ' ';
  sStatus[width] = '\0';
  pChangedPos = strchr(sStatus, '*');

  pStatLn = sStatus;
  c = GetColor(coStatus);
  i = 0;
  while (*pStatLn)
  {
    if (pStatLn == pChangedPos)
    {
      if (pFile->bRecoveryStored)
        disp_cbuf_put_char_attr(disp, x, i, *pStatLn, GetColor(coRecStored));
      else
        disp_cbuf_put_char_attr(disp, x, i, *pStatLn, c);
    }
    else
      disp_cbuf_put_char_attr(disp, x, i, *pStatLn, c);
    ++i;
    ++pStatLn;
  }

  c = GetColor(coTabs);  /* Only some tabs should be displayed below */
  /* If in column block mode */
  if (((pFile->blockattr & COLUMN_BLOCK) != 0) && pFile->bDisplayColumnMode)
  {
    i = width - 11;
    pStatLn = " COL ";
    while (*pStatLn)
      disp_cbuf_put_char_attr(disp, x, i++, *pStatLn++, c);
  }

  /* Display if the file is read only */
  if ((pFile->bReadOnly || pFile->bForceReadOnly) && pFile->bDisplayReadOnly)
  {
    i = width - 9;
    pStatLn = " R ";
    while (*pStatLn)
    {
      if (pFile->bReadOnly)
        disp_cbuf_put_char_attr(disp, x, i++, *pStatLn++, GetColor(coReadOnly));
      else   /* bForceReadOnly = TRUE */
        disp_cbuf_put_char_attr(disp, x, i++, *pStatLn++, c);
    }
  }
  else  /* Insert/Overtwrite is valid only in non-read only mode */
    if (!bInsert && pFile->bDisplayInsertMode)
    {
      i = width - 9;
      pStatLn = " OVR ";
      while (*pStatLn)
        disp_cbuf_put_char_attr(disp, x, i++, *pStatLn++, c);
    }

  /* Put the composed line from the buffer to the screen */
  disp_put_block(disp, 0, cur_height, width, 1, x);

_exit:
  disp_cbuf_mark_invalid(disp, x);  /* only for extra safety */
  if (x != OutputBuf)
    s_free(x);
  return 1;
}

/* ************************************************************************
   Function: ConsoleMessageFunc
   Description:
     MessageBox call-back function for console applications.
     Uses the bottom line to display messages.
     pFileName is optional parameter that replaces "(filename)" in fmt.
     pFileName is given separately so it could be shrunk to fit in
     certain width constraints.

  MSG_STATONLY -- display on a status line and exit.
  MSG_INFO/MSG_ERROR -- message class (and colors).
  MSG_ERRNO -- display an error message based on errno variable. In
    this case MSG_ERROR should be supplyed as well to provide the proper
    message class.
  MSG_OK or MSG_YESNO or MSG_YESNOCANCEL -- opposed to MSG_STATONLY,
    displays a message and waits for the proper key to be pressed; (stores
    and restores the status line -- console version). MSG_ERROR works
    as if MSG_OK is supplied.
*/
int ConsoleMessageProc(dispc_t *disp, const char *pTitle, WORD flags, const char *pFileName,
  const char *fmt, ...)
{
  va_list ap;
  char msgbuf[256];
  char filenamebuf[_MAX_PATH];
  char *p;
  int nmsglen;
  int nmsg2len;  /* Auxiliary message length */
  int filenamelen;
  void *pStatusLn;
  BYTE attr;
  int nExitCode;
  int Key;
  int cur_width;
  int cur_height;
  disp_event_t ev;

  cur_width = disp_wnd_get_width(disp) - 1;
  cur_height = disp_wnd_get_height(disp) - 1;

  if (pTitle != NULL)
  {
    /* Title is not supported in console version */
  }

  if (flags & MSG_ERROR)
  {
    if ((flags & (MSG_OK | MSG_YESNO | MSG_YESNOCANCEL)) == 0)
     flags |= MSG_OK;
  }

  /*
  Determine nmsg2len    depending on flags
  */
  nmsg2len = 0;
  if (flags & MSG_OK)
    nmsg2len = strlen(sPressEsc);
  if (flags & MSG_YESNO)
    nmsg2len = strlen(sAskYN);
  if (flags & MSG_YESNOCANCEL)
    nmsg2len = strlen(sAskYNEsc);

  msgbuf[0] = '\0';

  if ((flags & MSG_ERRNO) != 0)
  {
    /*
    Prepare errno message.
    */
    ASSERT(fmt == NULL);
    if (pFileName != NULL)
    {
      strcpy(msgbuf, sErrnoMsg);
      #ifdef DOS
      strcat(msgbuf, sys_errlist[errno]);
      #else
      {
        const char *psErrorMsg;
        char sBuf[10];

        psErrorMsg = strerror(errno);
        if (psErrorMsg == NULL)
        {
          sprintf(sBuf, "no error message for this error code: %#x", errno);
          psErrorMsg = sBuf;
        }
        strcat(msgbuf, strerror(errno));
      }
      #endif
    }
  }
  else
  {
    /*
    Compose normal message
    */
    va_start(ap, fmt);
    vsprintf(msgbuf, fmt, ap);
    va_end(ap);
  }

  if (pFileName != NULL)
  {
    nmsglen = strlen(msgbuf);
    p = strstr(msgbuf, FILE_NAME_FMT);
    if (p == NULL)
      goto _put_message;

    /*
    Remove FILE_NAME_FMT from the message.
    */
    memcpy(p, p + sizeof(FILE_NAME_FMT) - 1,
      nmsglen - (p - msgbuf) - (sizeof(FILE_NAME_FMT) - 1) + 1);
    nmsglen = strlen(msgbuf);

    /*
    Prepare pFileName in filenamebuf.
    */
    ShrinkPath(pFileName, filenamebuf, cur_width - nmsglen - nmsg2len - 1, FALSE);
    filenamelen = strlen(filenamebuf);

    /*
    Insert file name at the position where FILE_NAME_FMT was.
    */
    memmove(p + filenamelen, p, nmsglen - (p - msgbuf) + 1);  /* make room */
    memcpy(p, filenamebuf, filenamelen);
  }

_put_message:
  /*
  Add additional messages.
  */
  if (flags & MSG_OK)
    strcat(msgbuf, sPressEsc);
  if (flags & MSG_YESNO)
    strcat(msgbuf, sAskYN);
  if (flags & MSG_YESNOCANCEL)
    strcat(msgbuf, sAskYNEsc);

  if ((flags & MSG_STATONLY) == 0)
    pStatusLn = SaveStatusLine(disp);

  if ((flags & MSG_STATONLY) != 0)
    disp_cursor_goto_xy(disp, strlen(msgbuf), cur_height);

  attr = coStatus;
  if ((flags & MSG_WARNING) || (flags & MSG_ERROR))
    attr = coError;

  DisplayStatusStr(msgbuf, attr, TRUE, TRUE, disp);

  nExitCode = -1;
  if ((flags & (MSG_OK | MSG_YESNO | MSG_YESNOCANCEL)) != 0)
  {
    ASSERT((flags & MSG_STATONLY) == 0);

    /*
    Wait for a key.
    */
    while (nExitCode == -1)
    {
      do
      {
        disp_event_read(disp, &ev);
      }
      while (ev.t.code != EVENT_KEY);
      Key = ev.e.kbd.key;
      switch (Key)
      {
        case KEY(0, kbEsc):
          if ((flags & MSG_OK) != 0 || (flags & MSG_YESNOCANCEL) != 0)
            nExitCode = 2;
          break;
        case KEY(0, kbEnter):
          if ((flags & MSG_YESNO) != 0 || (flags & MSG_YESNOCANCEL) != 0)
            nExitCode = 0;
          break;
        default:
          if ((flags & MSG_YESNO) != 0 || (flags & MSG_YESNOCANCEL) != 0)
            switch (ASC(Key))
            {
              case 'y':
              case 'Y':
                nExitCode = 0;
                break;
              case 'n':
              case 'N':
                nExitCode = 1;
                break;
            }
      }
    }
    RestoreStatusLine(pStatusLn, disp);
  }
  return (nExitCode);
}

/* ************************************************************************
   Function: DisposeUserScreen
   Description:
     Disposes the user sceen buffer.
*/
void DisposeUserScreen(void)
{
#if 0
  if (pUserScreenBuf != NULL)
    s_free(pUserScreenBuf);
#endif
}

/* ************************************************************************
   Function: SaveUserScreen
   Description:
     Saves the user screen content and cursor position.
*/
void SaveUserScreen(void)
{
#ifndef _NON_TEXT
#if 0
  DisposeUserScreen();  /* Dispose old buffer (if any) */

  pUserScreenBuf = s_alloc(CalcRectSz(ScreenWidth, ScreenHeight));

  ASSERT(pUserScreenBuf != NULL);

  gettextblock(0, 0, CurWidth, CurHeight, pUserScreenBuf);
  GetCursorParam(&UserCursor);
#endif
#endif
}

/* ************************************************************************
   Function: ShowUserScreen
   Description:
     Displays the user screen and cursor position.
*/
void ShowUserScreen(void)
{
#ifndef _NON_TEXT
#if 0
  puttextblock(0, 0, CurWidth, CurHeight, pUserScreenBuf);
  RestoreCursor(&UserCursor);
#endif
#endif
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

