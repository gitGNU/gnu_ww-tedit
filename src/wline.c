/*

File: wline.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 2nd November, 1998
Descrition:
  Single line output (in buffer). Syntax highlighting.

*/

#include "global.h"
#include "l1opt.h"
#include "nav.h"
#include "synh.h"
#include "wline.h"

static int nOutputIndex;

/* ************************************************************************
   Function: SetBlockPos
   Description:
     If there's a block at the current line returns TRUE
     and nBlockStart and nBlockEnd marks the region of the
     line that should be filled with block attributes.
*/
static BOOLEAN SetBlockPos(const TFile *pFile, int nWrtLine,
  int *nBlockStart, int *nBlockEnd)
{
  const char *p;
  int n;
  int nEndMark;

  if (!LineIsInBlock(pFile, nWrtLine))
    return (FALSE);

  *nBlockStart = *nBlockEnd = -1;  /* Invalid by default */

  if (pFile->blockattr & COLUMN_BLOCK)
  {
    ASSERT(pFile->nStartPos >= 0);
    ASSERT(pFile->nEndPos >= 0);
    ASSERT(pFile->nEndPos >= pFile->nStartPos);

    *nBlockStart = pFile->nStartPos;
    *nBlockEnd = pFile->nEndPos;
  }
  else
  {
    ASSERT(pFile->nStartPos >= 0);
    ASSERT(pFile->nEndPos >= -1);

    /*
    Check the cases when nWrtLine is start block line
    and nWrtLine is end block line
    */
    if (nWrtLine == pFile->nStartLine)
      *nBlockStart = GetTabPos(pFile, nWrtLine,	pFile->nStartPos);
    if (nWrtLine == pFile->nEndLine)
    {
      if (pFile->nEndPos == -1)
        return FALSE;  /* Block ends inclusively at the end of the prev line */
      p = GetLineText(pFile, nWrtLine) + pFile->nEndPos;
      nEndMark = pFile->nEndPos;
      if (*p == '\t')
        ++nEndMark;  /* Tab char width to be included as well */
      n = GetTabPos(pFile, nWrtLine, nEndMark);
      if (*p == '\t')
        --n;
      *nBlockEnd = n;
    }

    if (*nBlockStart == -1)
      *nBlockStart = 0;  /* Line starts in a some previous line */
    if (*nBlockEnd == -1)
      *nBlockEnd = nOutputIndex;  /* Line ends in a some next line */
  }

  return TRUE;
}

/* ************************************************************************
   Function: PutCharAttr
   Description:
     Stores character/attributes pair if in visible region of a line.
     Visible region of a line this is left from nWrtEdge and
     up to nWinWidth of characters.
*/
static void PutCharAttr(char c, int attr, int nWrtEdge, int nWinWidth,
  TLineOutput *pOutputBuf)
{
  /*
  Check whether nOutputIndex falls in the visible section of the line
  */
  if (nOutputIndex < nWrtEdge)
    goto _exit;
  if (nOutputIndex - nWrtEdge > nWinWidth)
    goto _exit;

  pOutputBuf[nOutputIndex - nWrtEdge].c = c;
  pOutputBuf[nOutputIndex - nWrtEdge].t = attr;

_exit:
  ++nOutputIndex;
}

/* ************************************************************************
   Function: PutAttr
   Description:
     Fills a region of output buffer with specified attributes.
*/
static void PutAttr(int attr, int nWrtEdge, int nWinWidth,
  int nRegionStart, int nRegionEnd, TLineOutput *pOutputBuf)
{
  int nStartPos;
  int nEndPos;
  int i;

  ASSERT(nRegionStart <= nRegionEnd);

  nStartPos = nRegionStart  - nWrtEdge;
  nEndPos = nRegionEnd - nWrtEdge;

  if (nStartPos < 0 && nEndPos < 0)
    return;

  if (nStartPos < 0)
  {
    nStartPos = 0;
    ASSERT(nEndPos >= 0);
  }

  for (i = nStartPos; i <= nEndPos && i < nWinWidth; ++i)
    pOutputBuf[i].t = attr;
}

/* ************************************************************************
   Function: PutAttrWrap
   Description:
     Calls PutAttr(). Serves as a wrap for the syntax highlighting function
     to apply color attributes.
     nRegionEnd is inclusive end of range!
*/
static void PutAttrWrap(int attr, int nRegionStart, int nRegionEnd,
    struct SynHInterf *pContext)
{
  TLine *pLine;

  if (pContext->pOutputBuf == NULL)  /* dummy run? */
    return;

  pLine = pContext->pLine;
  /*
  nRegionStart..nRegionEnd are character coordinates and in pOutputBuf
  we have column coordinates (column != char when line contains tabs '\t')
  */
  nRegionStart = LineGetTabPos(pLine->pLine, nRegionStart);
  nRegionEnd = LineGetTabPos(pLine->pLine, nRegionEnd);
  PutAttr(attr, pContext->nWrtEdge, pContext->nWinWidth,
    nRegionStart, nRegionEnd, pContext->pOutputBuf);
}

/* ************************************************************************
   Function: GetPrevLineStatus
   Description:
     Syntax highlighting requires status of the previous line.
     Status of previous line should be set (SYNTAX_STATUS_SET).
     This function traverses the lines in reverse order starting from
     the current line to find a line with SYNTAX_STATUS_SET. Then it
     calls pfnApplyColors just to establish correct syntax status.
     It will eventually reach the current line and will carry the
     correct syntax status
*/
static int GetPrevLineStatus(const TFile *pFile, int Line)
{
  TLine *pLine;
  int WorkLine;
  int (*pfnApplyColors)(char *line, int len,
    int prevln_status, TSynHInterf *pApplyInterf);
  TSynHInterf stApplyInterf;
  int Status;

  if (Line == 0)  /* No previous line? */
    return 0;

  pfnApplyColors = GetSyntaxProc(pFile->nType);
  if (pfnApplyColors == NULL)  /* No syntax highlighting for this file? */
    return 0;

  stApplyInterf.nWinWidth = 0;
  stApplyInterf.nWrtEdge = 0;
  stApplyInterf.pOutputBuf = NULL;
  stApplyInterf.pfnPutAttr = PutAttrWrap;  /* Will work in a dummy mode */

  WorkLine = Line;

  if (WorkLine == pFile->nNumberOfLines + 1)  /* at the <*** end-of-file ***>? */
    return 0;

  ASSERT(WorkLine < pFile->nNumberOfLines + 1);

  /*
  Traverse back to find a line with SYNTAX_STATUS_SET
  */
  --WorkLine;
  Status = 0;
  while (WorkLine > 0)
  {
    pLine = GetLine(pFile, WorkLine);
    if ((pLine->attr & SYNTAX_STATUS_SET) != 0)
    {
      Status = LINE_SYNTAX_STATUS(pLine->attr);
      ++WorkLine;  /* Produce new status starting from the next line */
      break;
    }
    --WorkLine;
  }

  /*
  Start from WorkLine moving toward (Line-1) carrying the status
  */
  while (WorkLine <= Line - 1)
  {
    pLine = GetLine(pFile, WorkLine);
    Status = pfnApplyColors(pLine->pLine, pLine->nLen, Status, &stApplyInterf);
    pLine->attr = (Status | SYNTAX_STATUS_SET);
    ++WorkLine;
  }

  pLine = GetLine(pFile, Line - 1);
  return LINE_SYNTAX_STATUS(pLine->attr);
}


/* ************************************************************************
   Function: should_show_tooltip
   Description:
     Returns TRUE if nWrtLine is should be a tooltip line
*/
static int should_show_tooltip(const TFile *pFile, int nWrtLine,
  char **pTooltipLn)
{
  int nYLine;
  int nStartLine;
  int nTooltipLine;
  char *ln;
  char *buf;
  char c;
  BOOLEAN bQuit;

  if (pFile->nNumTooltipLines == 0)
    return FALSE;
  if (pFile->bTooltipIsTop)
  {
    nYLine = nWrtLine - pFile->nTopLine;
    if (nYLine > pFile->nNumTooltipLines - 1)
      return FALSE;
    nTooltipLine = 0;
    buf = pFile->sTooltipBuf;
    ln = buf;
  }
  else
  {
    nYLine = nWrtLine - pFile->nTopLine;
    nStartLine = pFile->nNumVisibleLines - pFile->nNumTooltipLines;
    if (nYLine < nStartLine)
      return FALSE;
    nTooltipLine = nYLine - nStartLine;
  }

  buf = pFile->sTooltipBuf;
  ln = buf;
  bQuit = FALSE;
  while (!bQuit)
  {
_scan_char:
    c = *buf;
    ++buf;
    if (c == '\n')
    {
      goto _end_of_loop;
    }
    if (c == '\0')
    {
      break;
    }
    goto _scan_char;
_end_of_loop:
    if (pFile->bTooltipIsTop)
    {
      ++nTooltipLine;
      bQuit = (nTooltipLine > nYLine);
    }
    else
    {
      --nTooltipLine;
      bQuit = (nTooltipLine < 0);
    }
    if (!bQuit)
      ln = buf;
  }
  *pTooltipLn = ln;
  return TRUE;
}

#define in_region(pos, start, end)  (pos >= start && pos <= end)

/* ************************************************************************
   Function: pos_in_hregions
   Description:
     Returns TRUE if pos falls in the highlight regions of a file
*/
static int pos_in_hregions(const TFile *pFile, int nPos)
{
  int i;

  for (i = 0; i < 6; ++i)
  {
    if (pFile->hlightareas[i].l == 0)
      return FALSE;
    if (in_region(nPos,
      pFile->hlightareas[i].c,
      pFile->hlightareas[i].c + pFile->hlightareas[i].l - 1))
      return TRUE;
  }
  return FALSE;
}

/* ************************************************************************
   Function: wline
   Description:
     Prepares a line for display output. Applaies syntax highlithing.
*/
void wline(const TFile *pFile, int nWrtLine, int nWidth,
  TLineOutput *pOutputBuf, TExtraColorInterf *pExtraColorInterf)
{
  const char *pTxt;
  TLine *pLine;
  int nFillUp;
  int nBlockStart;
  int nBlockEnd;
  int (*pfnApplyColors)(char *line, int len,
    int prevln_status, TSynHInterf *pApplyInterf);
  TSynHInterf stApplyInterf;
  int PrevLnStat;
  int Stat;
  int i;
  int pos;
  char *pTooltip;

  ASSERT(pOutputBuf != NULL);
  ASSERT(VALID_PFILE(pFile));
  ASSERT(nWrtLine >= 0);
  ASSERT(nWrtLine < pFile->nNumberOfLines);
  ASSERT(nWidth	> 0);
  ASSERT(nWidth < MAX_WIN_WIDTH);

  /*
  Check whether to show tooltip lines
  */
  if (should_show_tooltip(pFile, nWrtLine, &pTooltip))
  {
    nOutputIndex = 0;
    pTxt = pTooltip;

    while (*pTxt != '\0' && *pTxt != '\n')
    {
      if (*pTxt == '\t')
      {
        nFillUp = CalcTab(nOutputIndex) - nOutputIndex;
        while (nFillUp--)
          PutCharAttr(' ', attrTooltip, pFile->nWrtEdge, nWidth, pOutputBuf);
        ++pTxt;  /* Tab char was processed */
      }
      else
      {
        PutCharAttr(*pTxt++, attrTooltip, pFile->nWrtEdge, nWidth, pOutputBuf);
      }

      if (nOutputIndex - pFile->nWrtEdge > nWidth)
        break;
    }

    nFillUp = nOutputIndex;
    while (nFillUp++ <= nWidth + pFile->nWrtEdge)
      PutCharAttr(' ', attrTooltip, pFile->nWrtEdge, nWidth, pOutputBuf);
    goto _put_block;
  }

  nOutputIndex = 0;
  pTxt = GetLineText(pFile, nWrtLine);

  while (*pTxt != '\0')
  {
    if (*pTxt == '\t')
    {
      nFillUp = CalcTab(nOutputIndex) - nOutputIndex;
      while (nFillUp--)
        PutCharAttr(' ', attrText, pFile->nWrtEdge, nWidth, pOutputBuf);
      ++pTxt;  /* Tab char was processed */
    }
    else
    {
      PutCharAttr(*pTxt++, attrText, pFile->nWrtEdge, nWidth, pOutputBuf);
    }

    if (nOutputIndex - pFile->nWrtEdge > nWidth)
      break;
  }

  nFillUp = nOutputIndex;
  while (nFillUp++ <= nWidth + pFile->nWrtEdge)
    PutCharAttr(' ', attrText, pFile->nWrtEdge, nWidth, pOutputBuf);

  /* Apply syntax colors */
  pfnApplyColors = GetSyntaxProc(pFile->nType);
  pLine = GetLine(pFile, nWrtLine);
  if (pfnApplyColors != NULL)
  {
    stApplyInterf.nWinWidth = nWidth;
    stApplyInterf.nWrtEdge = pFile->nWrtEdge;
    stApplyInterf.pOutputBuf = pOutputBuf;
    stApplyInterf.pfnPutAttr = PutAttrWrap;
    stApplyInterf.pLine = pLine;
    PrevLnStat = GetPrevLineStatus(pFile, nWrtLine);
    Stat = pfnApplyColors(pLine->pLine, pLine->nLen, PrevLnStat, &stApplyInterf);
    pLine->attr = (Stat | SYNTAX_STATUS_SET);
  }

  /* Apply extra (higher level logic) colors */
  if (pExtraColorInterf)
  {
    stApplyInterf.nWinWidth = nWidth;
    stApplyInterf.nWrtEdge = pFile->nWrtEdge;
    stApplyInterf.pOutputBuf = pOutputBuf;
    stApplyInterf.pfnPutAttr = PutAttrWrap;
    stApplyInterf.pLine = pLine;
    pExtraColorInterf->pfnApplyExtraColors(NULL, FALSE, nWrtLine, 0, 0,
      &stApplyInterf, pExtraColorInterf);
  }

  /*
  Apply highligh color regions
  */
  for (i = 0; i < 6; ++i)
  {
    if (pFile->hlightareas[i].l == 0)
      break;
    if (pFile->hlightareas[i].r != nWrtLine)
      continue;

    pos = GetTabPos(pFile, nWrtLine, pFile->hlightareas[i].c);
    PutAttr(attrPair, pFile->nWrtEdge, nWidth,
      pos, pos + pFile->hlightareas[i].l - 1,
      pOutputBuf);
  }

_put_block:
  /* Put block attributes */
  if (SetBlockPos(pFile, nWrtLine, &nBlockStart, &nBlockEnd))
    PutAttr(attrBlock, pFile->nWrtEdge, nWidth,
      nBlockStart, nBlockEnd, pOutputBuf);

  if (pFile->bShowBlockCursor)
  {
    if (pFile->nRow == nWrtLine)
      PutAttr(attrBlockCursor, pFile->nWrtEdge, nWidth,
        pFile->nCol, pFile->nCol, pOutputBuf);
  }
}

/* ************************************************************************
   Function: GetEOLStatus
   Description:
     Returns the syntax highlighting status for this line (end of this line)
*/
DWORD GetEOLStatus(TFile *pFile, int Line)
{
  /*  We just use GetPrevLineStatus() for Line+1, */
  /* +it will extract correct status leading from */
  /* +the first line of the file                  */
  return GetPrevLineStatus(pFile, Line + 1);
}

/* ************************************************************************
   Function: GetLineStatusProc
   Description:
     Returns the syntax highlighting status for this line (end of this line)
*/
static DWORD GetLineStatusProc(int Line, struct LinesNavInterf *pContext)
{
  /*  We just use GetPrevLineStatus() for Line+1, */
  /* +it will extract correct status leading from */
  /* +the first line of the file                  */
  return GetEOLStatus(pContext->pFile, Line);
}

/* ************************************************************************
   Function: GetLineProc
   Description:
*/
static char *GetLineProc(int Line, struct LinesNavInterf *pContext)
{
  return GetLineText(pContext->pFile, Line);
}

/* ************************************************************************
   Function: GetLineLenProc
   Description:
*/
static int GetLineLenProc(int Line, struct LinesNavInterf *pContext)
{
  return GetLine(pContext->pFile, Line)->nLen;
}

/* ************************************************************************
   Function: FunctionNameScan
   Description:
     Scans region from a file for function names.
   Returns:
     FuncNames[] is populated with function names from the
        specified file region. FuncNames[].nLine of -1 marks end of list.
*/
int FunctionNameScan(const TFile *pFile,
  int nStartLine, int nStartPos,
  int nNumLines, int nEndLine,
  TFunctionName FuncNames[], int nMaxEntries)
{
  int (*pfnFuncNameScan)(int nStartLine, int nStartPos,
	int nNumLines, int nEndLine,
    TFunctionName FuncNames[], int nMaxEntries,
    TLinesNavInterf *pNavInterf);
  TLinesNavInterf stNavInterf;

  if (nEndLine == 0)
	nEndLine = nStartLine + nNumLines;

  pfnFuncNameScan = GetFuncNameScanProc(pFile->nType);
  if (pfnFuncNameScan == NULL)
    return 0;

  if (nStartLine + nNumLines > pFile->nNumberOfLines)
	nNumLines = pFile->nNumberOfLines - nStartLine;

  stNavInterf.pFile = (void *)pFile;
  stNavInterf.pfnGetLine = GetLineProc;
  stNavInterf.pfnGetLineLen = GetLineLenProc;
  stNavInterf.pfnGetLineStatus = GetLineStatusProc;
  return pfnFuncNameScan(nStartLine,
    nStartPos, nNumLines, nEndLine, FuncNames, nMaxEntries, &stNavInterf);
}

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2002, 2003
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

