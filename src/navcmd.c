/*

File: navcmd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 9th November, 1998
Descrition:
  Navigation commands.

*/

#include "global.h"
#include "l1def.h"
#include "l2disp.h"
#include "scr.h"
#include "nav.h"
#include "enterln.h"
#include "options.h"
#include "ini.h"  /* ValStr() */
#include "navcmd.h"
#include "wrkspace.h"

/* ************************************************************************
   Function: CmdEditCharLeft
   Description:
*/
void CmdEditCharLeft(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = pCtx;
  if (!bPersistentBlocks)
    LeapThroughSelection(pCurFile, FALSE);
  CharLeft(pCurFile);
}

/* ************************************************************************
   Function: CmdEditCharRight
   Description:
*/
void CmdEditCharRight(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = pCtx;
  if (!bPersistentBlocks)
    LeapThroughSelection(pCurFile, TRUE);
  CharRight(pCurFile);
}

/* ************************************************************************
   Function: CmdEditLineUp
   Description:
*/
void CmdEditLineUp(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = pCtx;
  if (!bPersistentBlocks)
    LeapThroughSelection(pCurFile, FALSE);
  LineUp(pCurFile);
}

/* ************************************************************************
   Function: CmdEditLineDown
   Description:
*/
void CmdEditLineDown(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = pCtx;
  if (!bPersistentBlocks)
    LeapThroughSelection(pCurFile, TRUE);
  LineDown(pCurFile);
}

/* ************************************************************************
   Function: CmdNextWord
   Description:
*/
void CmdEditNextWord(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = pCtx;
  if (!bPersistentBlocks)
    LeapThroughSelection(pCurFile, TRUE);
  if (bBlockMarkMode)
    GotoNextWordOrWordEnd(pCurFile);
  else
    GotoNextWord(pCurFile);
}

/* ************************************************************************
   Function: CmdPrevWord
   Description:
*/
void CmdEditPrevWord(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = pCtx;
  if (!bPersistentBlocks)
    LeapThroughSelection(pCurFile, FALSE);
  GotoPrevWord(pCurFile);
}

/* ************************************************************************
   Function: CmdEditHome
   Description:
*/
void CmdEditHome(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = pCtx;
  if (!bPersistentBlocks)
    LeapThroughSelection(pCurFile, FALSE);
  GotoHomePosition(pCurFile);
}

/* ************************************************************************
   Function: CmdEditEnd
   Description:
*/
void CmdEditEnd(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = pCtx;
  if (!bPersistentBlocks)
    LeapThroughSelection(pCurFile, TRUE);
  GotoEndPosition(pCurFile);
}

/* ************************************************************************
   Function: CmdEditTopFile
   Description:
*/
void CmdEditTopFile(void *pCtx)
{
  GotoTop(pCtx);
}

/* ************************************************************************
   Function: CmdEditBottomFile
   Description:
*/
void CmdEditBottomFile(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = pCtx;
  GotoColRow(pCurFile, pCurFile->nCol, pCurFile->nNumberOfLines);
}

/* ************************************************************************
   Function: CmdEditPageUp
   Description:
*/
void CmdEditPageUp(void *pCtx)
{
  GotoPageUp(pCtx, ScreenHeight - 1);
}

/* ************************************************************************
   Function: CmdEditPageDown
   Description:
*/
void CmdEditPageDown(void *pCtx)
{
  GotoPage(pCtx, ScreenHeight - 1, FALSE);
}

/* ************************************************************************
   Function: CmdEditGotoLine
   Description:
*/
void CmdEditGotoLine(void *pCtx)
{
  static char sNumBuf[20] = "";
  int nLine;

  if (!EnterLn(sEnterLine, sNumBuf, sizeof(sNumBuf) - 1, NULL, NULL, NULL, NULL, FALSE))
    return;

  if (!ValStr(sNumBuf, &nLine, 10))
  {
    ConsoleMessageProc(NULL, MSG_ERROR, NULL, sInvalidNumber, sNumBuf);
    return;
  }

  GotoColRow(pCtx, 0, nLine - 1);
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

