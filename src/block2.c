/*

File: block.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 9th July, 1999
Descrition:
  Functions concerning text block storage and manipulation
  Part II

*/

#include "global.h"
#include "l1opt.h"
#include "l1def.h"
#include "nav.h"
#include "memory.h"
#include "undo.h"
#include "block.h"

/* ************************************************************************
   Function: Rearrange
   Description:
     Rearranges a group of lines.
     Lines[] contains the # of lines to be placed at what positions.

     The permutation algorithm is designed by
     Tzvetan Mikov and Stanislav Angelov.

     NOTE: pLines is only temporarily modified. The contents remains
     the same on exit!
*/
void Rearrange(TFile *pFile, int nNumberOfLines, int *pLines)
{
  int nStartLine;
  int i;
  int nDest;
  int j;
  TLine TempLine;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(pLines != NULL);
  ASSERT(nNumberOfLines > 0);

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return;

  /*
  Determine start line number in pLines
  */
  nStartLine = INT_MAX;
  for (i = 0; i < nNumberOfLines; ++i)
  {
    if (pLines[i] < nStartLine)
      nStartLine = pLines[i];
  }

#define MARK INT_MIN

  for (i = 0; i < nNumberOfLines; ++i)
    pLines[i] |= MARK;

  for (i = 0; i < nNumberOfLines; ++i)
  {
    if ((pLines[i] & MARK) == 0)
      continue;

    j = i;

    /*
    As pLines contains line numbers not starting from 0
    we subtract and add nStartLine to make the algorithm
    works properly
    */
    nDest = (pLines[j] &= ~MARK) - nStartLine;

    while (nDest != i)
    {
      /*
      Swap pFile->pIndex[j] to pFile->pIndex[nDest]
      Take care for nStartLine offset.
      */
      memcpy(&TempLine, &pFile->pIndex[j + nStartLine], sizeof(TLine));
      memcpy(&pFile->pIndex[j + nStartLine],
        &pFile->pIndex[nDest + nStartLine], sizeof(TLine));
      memcpy(&pFile->pIndex[nDest + nStartLine], &TempLine, sizeof(TLine));

      j = nDest;
      nDest = (pLines[j] &= ~MARK) - nStartLine;
    }
  }

  return;
}

/* ************************************************************************
   Function: RevertRearrange
   Description:
     Exact oposite of Rearrange() function.
*/
void RevertRearrange(TFile *pFile, int nNumberOfLines, int *pLines)
{
  int nStartLine;
  int i;
  int nPos;
  int nDest;
  TLine TempLine;

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return;

  /*
  Determine start line number in pLines
  */
  nStartLine = INT_MAX;
  for (i = 0; i < nNumberOfLines; ++i)
  {
    if (pLines[i] < nStartLine)
      nStartLine = pLines[i];
  }

  for (i = 0; i < nNumberOfLines; ++i)
    pLines[i] |= MARK;

  for (i = 0; i < nNumberOfLines; ++i)
  {
    if ((pLines[i] & MARK) == 0)
      continue;

    nPos = (pLines[i] &= ~MARK) - nStartLine;
    nDest = i;

    while (nPos != i)
    {
      /*
      Swap pFile->pIndex[nPos] to pFile->pIndex[nDest]
      Take care for nStartLine offset.
      */
      memcpy(&TempLine, &pFile->pIndex[nPos + nStartLine], sizeof(TLine));
      memcpy(&pFile->pIndex[nPos + nStartLine],
        &pFile->pIndex[nDest + nStartLine], sizeof(TLine));
      memcpy(&pFile->pIndex[nDest + nStartLine], &TempLine, sizeof(TLine));

      nPos = (pLines[nDest] &= ~MARK) - nStartLine;
      nDest = nPos;
    }
  }
}

/*
Static variables to pass parameters to compare_lines()
*/
static const TFile *_pFile;
static int _nKeyCol;
static int _nKeyColEnd;

/* ************************************************************************
   Function: upcase
   Description:
     Sets a character in upper case depending by bCaseSensitiveSort flag.
*/
static char upcase(char c)
{
  if (bCaseSensitiveSort)
    return c;
  return toupper(c);
}

/* ************************************************************************
   Function: compare_lines
   Description:
     Call-back function called by qsort() to compare two lines.
     The lines are initialy stored into an integer array. The contents
     of the array are line numbers from the file.
     The lines are compared in a specific range of characters
     starting at offset _nKeyCol and ending at _nKeyColEnd.
   NOTE:
     There's no guarantee that _nKeyCol position is not falling
     at a '\t' TAB characters position.	But I suppose this is not
     of such a great concern like column block delete/insert operations.
*/
static int compare_lines(const void *_l1, const void *_l2)
{
  const int *l1 = _l1;
  const int *l2 = _l2;
  const TLine *pLine1;
  const TLine *pLine2;
  const char *p1;
  const char *p2;
  int nCol;

  pLine1 = GetLine(_pFile, *l1);
  pLine2 = GetLine(_pFile, *l2);

  /*
  Walk the lines accounting for '\t' TAB characters
  and stop if getting to _nKeyCol or last character position.
  */
  p1 = pLine1->pLine;
  nCol = 0;
  while (*p1)
  {
    if (nCol >= _nKeyCol)
      break;
    if (*p1 == '\t')  /* Tab char */
      nCol = CalcTab(nCol);
    else
      nCol++;
    ++p1;
  }

  p2 = pLine2->pLine;
  nCol = 0;
  while (*p2)
  {
    if (nCol >= _nKeyCol)
      break;
    if (*p2 == '\t')  /* Tab char */
      nCol = CalcTab(nCol);
    else
      nCol++;
    ++p2;
  }

  /*
  Walk through both the lines while:
  -- finding difference
  -- reaching string end
  -- reaching _nKeyColEnd
  */
  while ((upcase(*p1) - upcase(*p2)) == 0 && *p1 && *p2 && (p1 - pLine1->pLine) < _nKeyColEnd)
    ++p1, ++p2;

  if (bAscendingSort)
    return upcase(*p1) - upcase(*p2);
  else
    return upcase(*p2) - upcase(*p1);
}

/* ************************************************************************
   Function: SortBlockPrim
   Description:
     Generates an array that represents a sorted sequence of
     lines.
     nKeyCol == -1 indicates no column block selection to be used as a key
*/
static int *SortBlockPrim(TFile *pFile, int nStartLine, int nEndLine,
  int nKeyCol, int nKeyColEnd)
{
  int *pLineArray;
  int i;
  int nNumberOfLines;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nEndLine - nStartLine > 0);

  nNumberOfLines = nEndLine - nStartLine + 1;
  pLineArray = alloc(sizeof(int *) * nNumberOfLines);
  if (pLineArray == NULL)
    return NULL;

  for (i = nStartLine; i <= nEndLine; ++i)
    pLineArray[i - nStartLine] = i;

  /* Pass parameters to compare_lines() */
  _pFile = pFile;
  _nKeyCol = nKeyCol;
  _nKeyColEnd = nKeyColEnd;
  if (_nKeyCol == -1)
  {
    _nKeyCol = 0;
    _nKeyColEnd = INT_MAX;
  }

  qsort(pLineArray, nNumberOfLines, sizeof(int *), compare_lines);

  return pLineArray;
}

/* ************************************************************************
   Function: SortCurrentBlock
   Description:
*/
void SortCurrentBlock(TFile *pFile)
{
  int *pLineArray;
  int nNumberOfLines;
  int nEndLine;
  int nUndoEntry;
  BOOLEAN bResult;

  ASSERT(VALID_PFILE(pFile));

  if (!pFile->bBlock)
    return;

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return;

  nNumberOfLines = pFile->nEndLine - pFile->nStartLine + 1;
  ASSERT(nNumberOfLines > 0);
  if (nNumberOfLines == 1)
    return;  /* single line block */

  /*
  Pass column block selection to be used as a key (if any selection available)
  */
  nEndLine = pFile->nEndLine;
  if (pFile->blockattr & COLUMN_BLOCK)
    pLineArray = SortBlockPrim(pFile,
      pFile->nStartLine, nEndLine, pFile->nStartPos, pFile->nEndPos);
  else
  {
    if (pFile->nEndPos == -1)
    {
      --nEndLine;
      --nNumberOfLines;
    }
    pLineArray = SortBlockPrim(pFile,
      pFile->nStartLine, nEndLine, -1, -1);
  }

  if (pLineArray == NULL)
    return;

  bResult = TRUE;
  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (!AddUndoRecord(pFile, acREARRANGE, FALSE))
  {
    bResult = FALSE;
    goto _exit_point;
  }

  Rearrange(pFile, nNumberOfLines, pLineArray);
  RecordUndoData(pFile, pLineArray, pFile->nStartLine, -1, nEndLine, -1);

  pFile->bUpdatePage = TRUE;

  sprintf(pFile->sMsg, sSorted, nNumberOfLines);

  pFile->bChanged = TRUE;
  pFile->bRecoveryStored = FALSE;

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);
}

/* ************************************************************************
   Function: TrimTrailingBlanks
   Description:
     Removes trailing blanks of all the lines in the range 
     nStartLine..nEndLine (nEndLine is not included, at least
     one line is processed)
   Returns:
     TRUE -- lines sucessfully processed, *pnBlanksRemoved contains
     the number of blank characters that were removed.
     FALSE -- operation failed (no memory)
*/
BOOLEAN TrimTrailingBlanks(TFile *pFile, int nStartLine, int nEndLine,
  int *pnBlanksRemoved)
{
  int i;
  int nUndoEntry;
  BOOLEAN bResult;
  char *p;
  char *pLine;
  char *pEndOfLine;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nStartLine >= 0);
  ASSERT(nEndLine <= pFile->nNumberOfLines);

  nEndLine = pFile->nEndLine;
  if (pFile->nEndPos == -1)
    --nEndLine;
  ASSERT(nEndLine >= 0);

  bResult = TRUE;
  nUndoEntry = UNDO_BLOCK_BEGIN();

  *pnBlanksRemoved = 0;
  for (i = nStartLine; i <= nEndLine; ++i)
  {
    pLine = GetLineText(pFile, i);
    if (pLine[0] == '\0')
      continue;  /* line is empty */
    p = strchr(pLine, '\0');
    ASSERT(p != NULL);
    pEndOfLine = p;
    --p;
    while (isspace(*p))
    {
      if (p == pLine)
        break;
      --p;
    }
    if (!isspace(*p))
      ++p;
    if (p == pEndOfLine)
      continue;
    bResult = DeleteCharacterBlock(pFile, i, p - pLine, i, pEndOfLine - pLine - 1);
    if (!bResult)
      goto _exit_point;
    *pnBlanksRemoved += pEndOfLine - p;
  }

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);

  return bResult;
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

