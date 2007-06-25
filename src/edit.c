/*

File: edit.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 6th October, 1999
Descrition:
  Edit functions

*/

#include "global.h"
#include "file.h"
#include "nav.h"
#include "block.h"
#include "undo.h"
#include "search.h"
#include "edit.h"

/* ************************************************************************
   Function: Indent
   Description:
     Indents a line of text after pressing <Enter> to split the line.
*/
static BOOLEAN Indent(TFile *pFile)
{
  int i;
  char *pLine;
  char *p;
  int nIndentPos;
  int nPos;

  ASSERT(VALID_PFILE(pFile));

  if (!bAutoIndent)
    return TRUE;

  if (pFile->nRow == 0)
    return TRUE;

  for (i = pFile->nRow - 1; i >= 0; --i)
  {
    pLine = GetLineText(pFile, i);

    for (p = pLine; *p; ++p)
      if (!isspace(*p))
        break;

    if (*p == '\0')
      continue;

    nPos = INDEX_IN_LINE(pFile, i, p);
    nIndentPos = GetTabPos(pFile, i, nPos);
    if (nIndentPos == 0)
      break;  /* Nowhere to indent */
    if (*pFile->pCurPos != '\0')
      return InsertBlanks(pFile, -1, 0, nIndentPos);
    else
    {
      GotoColRow(pFile, nIndentPos, pFile->nRow);
      return TRUE;
    }
  }
  return TRUE;
}

/* ************************************************************************
   Function: SplitLine
   Description:
     Splits current line by inserting '\n'.
     Indents the line.
*/
void SplitLine(TFile *pFile)
{
  int nUndoEntry;
  BOOLEAN bResult;
  TBlock *pNewLineBlock;

  ASSERT(VALID_PFILE(pFile));

  nUndoEntry = UNDO_BLOCK_BEGIN();

  pNewLineBlock = MakeBlock("\n", 0);
  if (pNewLineBlock == NULL)
  {
    bResult = FALSE;
    goto _exit_point;
  }

  /*
  To prevent Paste() from inserting unnecessary spaces go at the end of the line
  */
  if (pFile->pCurPos != NULL)
  {
    if (*pFile->pCurPos == '\0')
      GotoPosRow(pFile, strlen(GetLineText(pFile, pFile->nRow)), pFile->nRow);
  }
  else
    GotoColRow(pFile, 0, pFile->nRow);

  bResult = Paste(pFile, pNewLineBlock);

  if (!bResult)
    goto _exit_point;

  bResult = Indent(pFile);

_exit_point:
  if (pNewLineBlock)
    DisposeABlock(&pNewLineBlock);  /* No longer necessary */
  UNDO_BLOCK_END(nUndoEntry, bResult);
}

/* ************************************************************************
   Function: Unindent
   Description:
     Unindents line if only blank characters remain left from
     cursor position. To be called from <Backspace> function.
   Returns:
     TRUE if the function produced an unindent.
     FALSE no unindent took place.
*/
static BOOLEAN Unindent(TFile *pFile, BOOLEAN *pbResult)
{
  char *p;
  int nLine;
  int nIndentPos;
  int nLastCol;

  if (!bBackspaceUnindent)
    return FALSE;

  if (pFile->nCol == 0)
    return FALSE;

  /*
  Determine whether the current line is suitable for backspace unindent.
  A line is suitable provided that there are only blank spaces before
  the cursor position.
  */
  p = GetLineText(pFile, pFile->nRow);
  nLastCol = 0;
  while (*p != '\0')
  {
    if (p == pFile->pCurPos)
      break;
    if (!isspace(*p))
      return FALSE;
    if (*p == '\t')
      nLastCol = CalcTab(nLastCol);
    else
      ++nLastCol;
    ++p;
  }

  /*
  Move through the lines that are above the current line searching
  for one that has a smaller leading blank space area.
  */
  nLine = pFile->nRow - 1;
  while (nLine >= 0)
  {
    p = GetLineText(pFile, nLine);
    nIndentPos = 0;
    while (*p != '\0')
    {
      if (nIndentPos >= pFile->nCol)
        goto _next_line;  /* This line is unsuitable */
      if (*p == '\t')  /* Tab char */
        nIndentPos = CalcTab(nIndentPos);
      else
        if (isspace(*p))
          ++nIndentPos;
        else
        {
          if (nIndentPos < pFile->nCol)
            goto _unindent;
          else
            break;
        }
      ++p;
    }
_next_line:
    --nLine;
  }

_unindent:
  /*
  Unindent at nIndentPos.
  We use the column function to detab and delete as unindent is
  not character oriented but column position oriented instead.
  note: Those function are invoked to operate on a single line here.
  */
  ASSERT(nIndentPos < pFile->nCol);
  if (nIndentPos > nLastCol)
  {
    /* Notning to remove, jump on a position */
    GotoColRow(pFile, nIndentPos, pFile->nRow);
    *pbResult = TRUE;
    return TRUE;
  }
  if (!DetabColumnRegion(pFile,
    pFile->nRow, nIndentPos, pFile->nRow, pFile->nCol - 1))
  {
    *pbResult = FALSE;
    return TRUE;
  }
  if (!DeleteColumnBlock(pFile,
    pFile->nRow, nIndentPos, pFile->nRow, pFile->nCol - 1))
  {
    *pbResult = FALSE;
    return TRUE;
  }
  *pbResult = TRUE;
  return TRUE;
}

/* ************************************************************************
   Function: Backspace
   Description:
*/
void Backspace(TFile *pFile, int nWidth, TSearchContext *pstSearchContext)
{
  int nCurPos;
  int nCol;
  int nUndoEntry;
  BOOLEAN bResult;

  ASSERT(VALID_PFILE(pFile));

  if (bIncrementalSearch)
  {
    bPreserveIncrementalSearch = TRUE;
    IncrementalSearchRemoveLast(pFile, nWidth, pstSearchContext);
    return;
  }

  if (bOverwriteBlocks)
  {
    if (pFile->bBlock)
    {
      DeleteBlock(pFile);
      return;
    }
  }

  if (pFile->pCurPos == NULL)
    return;  /* Nothing to delete at the very end line of the file, or file empty */

  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (Unindent(pFile, &bResult))
    goto _exit_point;  /* Unindent took place */

  bResult = TRUE;

  nCurPos = INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos);
  ASSERT(nCurPos >= 0);
  if (nCurPos == 0)
  {
    if (pFile->nCol > 0)
    {
      GotoColRow(pFile, pFile->nCol - 1, pFile->nRow);
      goto _exit_point;
    }
    if (pFile->nRow == 0)
      goto _exit_point;
    /* Join at the end of the prev line */
    bResult = DeleteCharacterBlock(pFile,
      pFile->nRow - 1, strlen(GetLineText(pFile, pFile->nRow - 1)),
      pFile->nRow, -1);
    goto _exit_point;
  }

  /*
  Detect whether inside tab area or outside the most left text position.
  */
  nCol = GetTabPos(pFile, pFile->nRow, nCurPos);
  if (nCol == pFile->nCol)
  {
    bResult = DeleteCharacterBlock(pFile,
      pFile->nRow, nCurPos - 1, pFile->nRow, nCurPos - 1);
    goto _exit_point;
  }
  if (nCurPos < (int)strlen(GetLineText(pFile, pFile->nRow)))
  {
    bResult = PrepareColRowPosition(pFile, NULL);
    if (bResult)
    {
      bResult = DeleteCharacterBlock(pFile,
      pFile->nRow, nCurPos - 1, pFile->nRow, nCurPos - 1);
    }
  }
  else
  {
    GotoColRow(pFile, pFile->nCol - 1, pFile->nRow);
  }
_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);
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

