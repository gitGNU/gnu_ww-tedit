/*

File: edinterf.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 6th May, 2004
Descrition:
  File editor interfaces.
  Functions and structures for accessing file editing functions. It is intended
  for use in plug-in-s.

*/

#include "global.h"
#include "file.h"
#include "synh.h"
#include "nav.h"
#include "wline.h"
#include "edinterf.h"

/* ************************************************************************
   Function: fnGetCurPos
   Description:
     Returns the cursor position in a file.
*/
static void fnGetCurPos(int *pnCol, int *pnRow, int *pnPos,
  struct EditInterf *pInterf)
{
  TFile *pFile;

  pFile = pInterf->pFile;
  ASSERT(VALID_PFILE(pFile));

  *pnCol = pFile->nCol;
  *pnRow = pFile->nRow;
  *pnPos = 0;
  if (pFile->pCurPos != NULL)
    *pnPos = INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos);
}

/* ************************************************************************
   Function: fnGotoColRow
   Description:
     Sets the cursor position in a file.
*/
static void fnGotoColRow(int nCol, int nRow, struct EditInterf *pInterf)
{
  TFile *pFile;

  pFile = pInterf->pFile;
  ASSERT(VALID_PFILE(pFile));

  GotoColRow(pFile, nCol, nRow);
}

/* ************************************************************************
   Function: fnGotoPosRow
   Description:
     Sets the cursor position in a file.
     Uses character position instead of column position.
     (they differ if the line contains \t tabs)
*/
static void fnGotoPosRow(int nPos, int nRow, struct EditInterf *pInterf)
{
  TFile *pFile;

  pFile = pInterf->pFile;
  ASSERT(VALID_PFILE(pFile));

  GotoPosRow(pFile, nRow, nPos);
}

/* ************************************************************************
   Function: fnGotoPosRow
   Description:
*/
int fnGetTabPos(int nPos, int nRow, struct EditInterf *pInterf)
{
  TFile *pFile;

  pFile = pInterf->pFile;
  ASSERT(VALID_PFILE(pFile));

  return GetTabPos(pFile, nRow, nPos);
}

/* ************************************************************************
   Function: fnGetLineStatus
   Description:
     Returns the syntax highlighting status for this line (end of this line)
*/
static DWORD fnGetLineStatus(int Line, struct LinesNavInterf *pContext)
{
  /*  We just use GetPrevLineStatus() for Line+1, */
  /* +it will extract correct status leading from */
  /* +the first line of the file                  */
  return GetEOLStatus(pContext->pFile, Line);
}

/* ************************************************************************
   Function: fnGetLine
   Description:
*/
static char *fnGetLine(int Line, struct LinesNavInterf *pContext)
{
  return GetLineText(pContext->pFile, Line);
}

/* ************************************************************************
   Function: fnGetLineLen
   Description:
*/
static int fnGetLineLen(int Line, struct LinesNavInterf *pContext)
{
  return GetLine(pContext->pFile, Line)->nLen;
}

/* ************************************************************************
   Function: fnGetNumLines
   Description:
     Returns the number of lines in a file.
*/
static int fnGetNumLines(TEditInterf *pInterf)
{
  TFile *pFile;

  pFile = pInterf->pFile;
  return pFile->nNumberOfLines;
}

/* ************************************************************************
   Function: fnGetText
   Description:
     Extracts portion of text from the file into pDest.
*/
static int fnGetText(int nLine, int nPos, int nLen, void *pDest,
  struct EditInterf *pInterf)
{
  char *p;

  TFile *pFile;

  pFile = pInterf->pFile;

  memset(pDest, 0, nLen);  
  if (nLine > pFile->nNumberOfLines)
    return 0;

  p = GetLineText(pFile, nLine);
  strncpy(pDest, p, nLen - 1);
  return 0;
}

/* ************************************************************************
   Function: fnGetCurPagePos
   Description:
     Return the coordinates of the top left corner of the portion
     of the file which is visible on the screen
*/
static void fnGetCurPagePos(int *pnTopLine, int *pnWrtEdge,
  int *pnNumVisibleLines, struct EditInterf *pInterf)
{
  TFile *pFile;

  pFile = pInterf->pFile;
  if (pnTopLine != NULL)
    *pnTopLine = pFile->nTopLine;
  if (pnWrtEdge != NULL)
    *pnWrtEdge = pFile->nWrtEdge;
  if (pnNumVisibleLines != NULL)
    *pnNumVisibleLines = pFile->nNumVisibleLines;
}

/* ************************************************************************
   Function: ProduceEditInterf
   Description:
     Prepares an interface for accessing the edit functions for a
     specific file.
*/
void ProduceEditInterf(TEditInterf *pInterf, TFile *pFile)
{
  memset(pInterf, 0, sizeof(TEditInterf));
  pInterf->pFile = pFile;
  pInterf->pfnGetCurPos = fnGetCurPos;
  pInterf->pfnGotoColRow = fnGotoColRow;
  pInterf->pfnGotoPosRow = fnGotoPosRow;
  pInterf->pfnGetTabPos = fnGetTabPos;
  pInterf->pfnGetNumLines = fnGetNumLines;
  pInterf->pfnGetText = fnGetText;
  pInterf->pfnGetCurPagePos = fnGetCurPagePos;

  pInterf->stNavInterf.pFile = (void *)pFile;
  pInterf->stNavInterf.pfnGetLine = fnGetLine;
  pInterf->stNavInterf.pfnGetLineLen = fnGetLineLen;
  pInterf->stNavInterf.pfnGetLineStatus = fnGetLineStatus;
}

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2004
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

