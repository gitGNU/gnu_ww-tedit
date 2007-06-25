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
#include "memory.h"
#include "scr.h"
#include "kbd.h"
#include "keydefs.h"
#include "nav.h"
#include "wline.h"
#include "palette.h"
#include "l1def.h"
#include "defs.h"
#include "path.h"
#include "l2disp.h"
#include "wrkspace.h"

static BYTE *pUserScreenBuf = NULL;
static CursorParam UserCursor;
static int UserCursorX, UserCursorY;

/* ************************************************************************
   Function: PutText
   Description:
     Call-back function to display a line. DJGPP and Win32 console
     application release.
*/
static int PutText(int nStartX, int nStartY, int nWidth, int nYLine, 
  int nPaletteStart, const TLineOutput *pBuf)
{
  CharInfo OutputBuf[MAX_WIN_WIDTH];
  CharInfo *p;
  const TLineOutput *b;
  int i;

  p = OutputBuf;
  b = pBuf;

  for (i = 0; i < ScreenWidth; ++i)
  {
    PutChar(p, b->c);
    PutAttr(p, GetColor(b->t + nPaletteStart));
    ++p;
    ++b;
  }

  puttextblock(nStartX, nStartY + nYLine, nStartX + nWidth - 1,
    nStartY + nYLine, OutputBuf);

  return (0);
}

/* ************************************************************************
   Function: ConsoleUpdatePage
   Description:
     Updates current page at the screen. DJGPP and Win32 console
     application wrap of UpdatePage().
*/
void ConsoleUpdatePage(TFile *pFile, int x1, int y1, int x2, int y2,
  int nPaletteStart, void *pExtraColorInterf)
{
  ASSERT(VALID_PFILE(pFile));

  UpdatePage(pFile, x1, y1, x2 - x1, y2 - y1, nPaletteStart, 
    &stSearchContext, PutText, pExtraColorInterf);
}

/* ************************************************************************
   Function: DisplayStatusStr
   Description:
     Displays a string at the status line.
     bClean = TRUE to clear the line before displaying the string.
     bShowCursor = TRUE to set the cursor at the end of the string.
*/
void DisplayStatusStr(const char *sStatus, BYTE attr, BOOLEAN bClean,
  BOOLEAN bShowCursor)
{
  ASSERT(sStatus != NULL);
  ASSERT(sStatus[0] != '\0');  /* Check for valid string -- not empty */
  ASSERT(strlen(sStatus) < MAX_WIN_WIDTH);  /* And with an end marker */

  if (bClean)
    FillXY(' ', GetColor(attr), 0, CurHeight, ScreenWidth);

  WriteXY(sStatus, 0, CurHeight, GetColor(attr));

  if (bShowCursor)
    GotoXY(strlen(sStatus), CurHeight);
}

/* ************************************************************************
   Function: DisplayStatusStr2
   Description:
     Displays status string with flex attribute.
     Example: '~T~est' will highlight 'T'.
*/
void DisplayStatusStr2(const char *sStatus, BYTE attr1, BYTE attr2)
{
  ASSERT(sStatus != NULL);
  ASSERT(sStatus[0] != '\0');  /* Check for valid string -- not empty */
  ASSERT(strlen(sStatus) < MAX_WIN_WIDTH);  /* And with an end marker */

  FillXY(' ', GetColor(attr1), 0, CurHeight, ScreenWidth);

  FlexWriteXY(sStatus, 0, CurHeight, GetColor(attr1), GetColor(attr2));
}

/* ************************************************************************
   Function: SaveStatusLine
   Description:
     Saves the status line in the heap.
     Returns a pointer to the image.
*/
void *SaveStatusLine(void)
{
  BYTE *pStatLineBuf;

  pStatLineBuf = s_alloc(CalcRectSz(ScreenWidth, 1));
  gettextblock(0, CurHeight, CurWidth, CurHeight, pStatLineBuf);
  return (pStatLineBuf);
}

/* ************************************************************************
   Function: RestoreStatusLine
   Description:
     Restores a status line image from the heap.
*/
void RestoreStatusLine(void *pStatus)
{
  puttextblock(0, CurHeight, CurWidth, CurHeight, pStatus);
  s_free(pStatus);
}

/* ************************************************************************
   Function: ConsoleUpdateStatusLine
   Description:
     Updates status line.
   Returns:
     0 - status line was not updated.
*/
int ConsoleUpdateStatusLine(TFile *pFile)
{
  char sStatus[255];
  char sChanged[2];
  CharInfo *Output;
  CharInfo WrtLnBuf[MAX_WIN_WIDTH];
  char *pStatLn;
  char *pChangedPos;
  BYTE c;
  char cSep;

  if (!pFile->bUpdateStatus)
    return 0;

  pFile->bUpdateStatus = FALSE;

  if (pFile->sMsg[0] != '\0')  /* Display the message only */
  {
    memset(sStatus, ' ', sizeof(sStatus));
    strcpy(sStatus, pFile->sMsg);
    *strchr(sStatus, '\0') = ' ';
    sStatus[ScreenWidth] = '\0';
    DisplayStatusStr(sStatus, coStatus, FALSE, FALSE);
    return 1;
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
  sStatus[ScreenWidth] = '\0';
  pChangedPos = strchr(sStatus, '*');

  Output = (CharInfo *)WrtLnBuf;  /* Use WrtLnBuf as temporary buffer */
  pStatLn = sStatus;
  c = GetColor(coStatus);
  while (*pStatLn)
  {
    PutChar(Output, *pStatLn);
    if (pStatLn++ == pChangedPos)
    {
      if (pFile->bRecoveryStored)
        PutAttr(Output, GetColor(coRecStored));
      else
        PutAttr(Output, c);
    }
    else
      PutAttr(Output, c);
    Output++;
  }

  c = GetColor(coTabs);  /* Only some tabs should be displayed below */
  /* If in column block mode */
  if (((pFile->blockattr & COLUMN_BLOCK) != 0) && pFile->bDisplayColumnMode)
  {
    Output = WrtLnBuf;
    Output += CurWidth - 12;
    pStatLn = " COL ";
    while (*pStatLn)
    {
      PutChar(Output, *pStatLn++);
      PutAttr(Output, c);
      Output++;
    }
  }

  /* Display if the file is read only */
  if ((pFile->bReadOnly || pFile->bForceReadOnly) && pFile->bDisplayReadOnly)
  {
    Output = WrtLnBuf;
    Output += CurWidth - 8;
    pStatLn = " R ";
    while (*pStatLn)
    {
      PutChar(Output, *pStatLn++);
      if (pFile->bReadOnly)
        PutAttr(Output, GetColor(coReadOnly));
      else
        PutAttr(Output, c);  /* bForceReadOnly = TRUE */
      Output++;
    }
  }
  else  /* Insert/Overtwrite is valid only in non-read only mode */
    if (!bInsert && pFile->bDisplayInsertMode)
    {
      Output = (CharInfo *)WrtLnBuf;
      Output += CurWidth - 8;
      pStatLn = " OVR ";
      while (*pStatLn)
      {
        PutChar(Output, *pStatLn++);
        PutAttr(Output, c);
        Output++;  /* PutChar() and PutAttr() are macroses that use Output */
      }
    }

  /* Put the composed line from WrtLnBuf */
  puttextblock(0, CurHeight, CurWidth, CurHeight, WrtLnBuf);
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
int ConsoleMessageProc(const char *pTitle, WORD flags, const char *pFileName,
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
  DWORD dwKey;

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
    ShrinkPath(pFileName, filenamebuf, CurWidth - nmsglen - nmsg2len - 1, FALSE);
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
    pStatusLn = SaveStatusLine();

  if ((flags & MSG_STATONLY) != 0)
    GotoXY(strlen(msgbuf), CurHeight);

  attr = coStatus;
  if ((flags & MSG_WARNING) || (flags & MSG_ERROR))
    attr = coError;

  DisplayStatusStr(msgbuf, attr, TRUE, TRUE);

  nExitCode = -1;
  if ((flags & (MSG_OK | MSG_YESNO | MSG_YESNOCANCEL)) != 0)
  {
    ASSERT((flags & MSG_STATONLY) == 0);

    /*
    Wait for a key.
    */
    while (nExitCode == -1)
    {
      switch (dwKey = ReadKey())
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
            switch (ASC(dwKey))
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
    RestoreStatusLine(pStatusLn);
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
  if (pUserScreenBuf != NULL)
    s_free(pUserScreenBuf);
}

/* ************************************************************************
   Function: SaveUserScreen
   Description:
     Saves the user screen content and cursor position.
*/
void SaveUserScreen(void)
{
#ifndef _NON_TEXT
  DisposeUserScreen();  /* Dispose old buffer (if any) */

  pUserScreenBuf = s_alloc(CalcRectSz(ScreenWidth, ScreenHeight));

  ASSERT(pUserScreenBuf != NULL);

  gettextblock(0, 0, CurWidth, CurHeight, pUserScreenBuf);
  GetCursorParam(&UserCursor);
  GetCursorXY(&UserCursorX, &UserCursorY);
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
  puttextblock(0, 0, CurWidth, CurHeight, pUserScreenBuf);
  RestoreCursor(&UserCursor);
  GotoXY(UserCursorX, UserCursorY);
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

