/*

File: nav.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 30th October, 1998
Descrition:
  File navigation, maintaining current position, current page updating,
  tab character manipulation functions,	goto-word positioning functions.

*/

#include "global.h"
#include "l1def.h"
#include "memory.h"
#include "wline.h"
#include "search.h"
#include "nav.h"

/* ************************************************************************
   Function: PrepareString
   Description:
     Prepares a string for output.
*/
static void PrepareString(const char *pMsg, int nWidth, char Type, TLineOutput *pBuf)
{
  const char *p;
  TLineOutput *b;

  p = pMsg;
  b = pBuf;

  while (*p)
  {
    b->c = *p;
    b->t = Type;
    ++p;
    ++b;

    if (b - pBuf >= nWidth)
      return;
  }

  while (b - pBuf < nWidth)
  {
    b->c = ' ';
    b->t = Type;
    ++b;
  }
}

/* ************************************************************************
   Function: WriteLine
   Description:
     Displays a line from a file.
   Parameters:
     nWrtLine - which line to display.
     nWidth - how much characters to contain the output buffer.
     PutText - call-back functions from Layer2 to actualy put the line
       on the screen.
   Returns:
     What PutText() returned as exit code.
*/
static int WriteLine(const TFile *pFile, int nStartX, int nStartY,
  int nWrtLine, int nWidth, int nPaletteStart,
  int (*PutText)(int nStartX, int nStartY, int nWidth, int nYLine,
  int nPaletteStart, const TLineOutput *pBuf, dispc_t *disp),
  TExtraColorInterf *pExtraColorInterf, dispc_t *disp)
{
  TLineOutput OutputBuf[MAX_WIN_WIDTH];
  int nYLine;

  nYLine = nWrtLine - pFile->nTopLine;
  ASSERT(nYLine >= 0);
  if (nWrtLine < pFile->nNumberOfLines)
    wline(pFile, nWrtLine, nWidth, OutputBuf, pExtraColorInterf);
  else
  {
    /*
    There's no line with number nWrtLine.
    Display <*** End Of File ***> or fill with empty lines.
    */
    if (nWrtLine == pFile->nNumberOfLines)
      PrepareString(pFile->sEndOfFile, nWidth, attrEOF, OutputBuf);
    else
      PrepareString("", nWidth, attrText, OutputBuf);
  }
  return (PutText(nStartX,
                  nStartY, nWidth, nYLine, nPaletteStart, OutputBuf, disp));
}

/* ************************************************************************
   Function: WritePage
   Description:
     Displays current page.
   Parameters:
     nWinHeight - height of a window where the current page should
       be displayed.
     nWinWidth - width of a window where the current page should
       be displayed.
     PutText - call-back functions from Layer2 to actualy put the line
       on the screen.
   Returns:
     What WriteLine()/PutText() returned as exit code.
*/
static int WritePage(const TFile *pFile, int nStartX, int nStartY,
  int nWinHeight, int nWinWidth, int nPaletteStart,
  int (*PutText)(int nStartX, int nStartY, int nWidth, int nYLine,
  int nPaletteStart, const TLineOutput *pBuf, dispc_t *disp),
  TExtraColorInterf *pExtraColorInterf, dispc_t *disp)
{
  int i;
  int nResult;

  if (pExtraColorInterf)  /* clear extra color interf cache */
  {
    pExtraColorInterf->pfnApplyExtraColors(pFile, TRUE, pFile->nTopLine,
      pFile->nTopLine, nWinHeight, NULL, pExtraColorInterf);
  }

  for (i = 0; i < nWinHeight; ++i)
  {
    nResult = WriteLine(pFile, nStartX, nStartY, i + pFile->nTopLine, nWinWidth,
      nPaletteStart, PutText, pExtraColorInterf, disp);
    if (nResult != 0)
      return (nResult);
  }
  return (0);
}

/* ************************************************************************
   Function: FixWrtPos
   Description:
     pnWrtPos is the first visible position. nCurPos is the current
     cursor position. nWidth is the size of the visible area.
     This function changes pnWrtPos so the character under cursor
     falls in the visible area.
*/
void FixWrtPos(int *nWrtPos, int nCurPos, int nWidth)
{
  if (nCurPos - *nWrtPos > nWidth - 2)
    *nWrtPos = nCurPos - nWidth + 2;
  else
    if (nCurPos < *nWrtPos)
      *nWrtPos = nCurPos;
}

/* ************************************************************************
   Function: FixPageCorner
   Description:
     This is the general function that makes current
     cursor position to be on the screen.
*/
static void FixPageCorner(TFile *pFile, int nWinWidth, int nWinHeight,
  const TSearchContext *pstSearchContext)
{
  int nOldWrtEdge;
  int nOldTopLine;
  BOOLEAN bUpdatePageInternal;
  int nMostRight;

  nOldWrtEdge = pFile->nWrtEdge;
  nOldTopLine = pFile->nTopLine;
  FixWrtPos(&pFile->nWrtEdge, pFile->nCol, nWinWidth);
  FixWrtPos(&pFile->nTopLine, pFile->nRow, nWinHeight);
  bUpdatePageInternal = FALSE;
  if (bIncrementalSearch)
  {
    /* We need to show the entire string on the screen */
    nMostRight = pFile->nCol + strlen(pstSearchContext->sSearch);
    if (pFile->nWrtEdge + nWinWidth < nMostRight + 2)
    {
      pFile->nWrtEdge = nMostRight - nWinWidth + 2;
      bUpdatePageInternal = TRUE;
    }
  }
  if (nOldTopLine != pFile->nTopLine)
    bUpdatePageInternal = TRUE;
  if (nOldWrtEdge != pFile->nWrtEdge)
    bUpdatePageInternal = TRUE;
  if (bUpdatePageInternal)
    pFile->bUpdatePage = TRUE;
}

/* ************************************************************************
   Function: UpdatePage
   Description:
     Fixes current position of the visible window.
     Checks if it's necessary to update current page of the file.
   Parameters:
     nWinHeight - height of a window where the current page should
       be displayed.
     nWinWidth - width of a window where the current page should
       be displayed.
     PutText - call-back functions from Layer2 to actualy put the line
       on the screen.
   Returns:
     What WritePage()/WriteLine()/PutText() returned as exit code.
*/
int UpdatePage(TFile *pFile, int nStartX, int nStartY, int nWinWidth, int nWinHeight,
  int nPaletteStart,
  const TSearchContext *pstSearchContext,
  int (*PutText)(int nStartX, int nStartY, int nWidth, int nYLine,
  int nPaletteStart, const TLineOutput *pBuf, dispc_t *disp),
  TExtraColorInterf *pExtraColorInterf, dispc_t *disp)
{
  int nResult;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PSEARCHCTX(pstSearchContext));
  ASSERT(nWinWidth > 0);
  ASSERT(nWinHeight > 0);
  ASSERT(PutText != NULL);

  FixPageCorner(pFile, nWinWidth, nWinHeight, pstSearchContext);
  nResult = 0;  /* In case bUpdatePage is FALSE */
  if (pFile->bUpdatePage)
    nResult = WritePage(pFile, nStartX, nStartY,
      nWinHeight, nWinWidth, nPaletteStart, PutText, pExtraColorInterf, disp);
  if (nResult != 0)
    goto _prepare_xy;
  if (pFile->bUpdateLine)
    nResult = WriteLine(pFile, nStartX, nStartY,
      pFile->nRow, nWinWidth, nPaletteStart, PutText, pExtraColorInterf, disp);
  if (nResult != 0)
    goto _prepare_xy;
  pFile->bUpdatePage = FALSE;
  pFile->bUpdateLine = FALSE;
_prepare_xy:
  pFile->y = pFile->nRow - pFile->nTopLine;
  pFile->x = pFile->nCol - pFile->nWrtEdge;
  if (bIncrementalSearch)
    pFile->x += strlen(pstSearchContext->sSearch);
  return (0);
}

/* ************************************************************************
   Function: CalcTab
   Description:
     Calculates new cursor position if at nPos there's a tab placed.
*/
int CalcTab(int nPos)
{
  return (nPos + nTabSize - nPos % nTabSize);
}

/* ************************************************************************
   Function: LineGetTabPos
   Description:
     Calculates column position by a given characters position
     for a text line counting the tabs.
   Returns:
     Column position for the corespondent characters position.
*/
int LineGetTabPos(const char *pLine, int nPos)
{
  const char *p;
  const char *pLineStart;
  int nNewPos;

  ASSERT(nPos >= 0);

  p = pLineStart = pLine;

  nNewPos = 0;
  while (*p)
  {
    if (p - pLineStart == nPos)
      break;
    if (*p == 9)  /* Tab char */
      nNewPos = CalcTab(nNewPos);
    else
      nNewPos++;
    p++;
  }

  return nNewPos;
}

/* ************************************************************************
   Function: GetTabPos
   Description:
     Calculates column position by a given characters position
     for a text line counting the tabs.
   Parameters:
     nLine -- line to precess characters potosition for.
     nPos -- character position in a line.
   Returns:
     Column position for the corespondent characters position.
*/
int GetTabPos(const TFile *pFile, int nLine, int nPos)
{
  ASSERT(VALID_PFILE(pFile));
  ASSERT(nLine >= 0);
  ASSERT(nLine <= pFile->nNumberOfLines);
  ASSERT(nPos >= 0);

  /* If we are at the <***EndOfFile***> line */
  if (nLine == pFile->nNumberOfLines)
    return (nPos);  /* The cursor is not affected by any tabs */

  return LineGetTabPos(GetLineText(pFile, nLine), nPos);
}

/* ************************************************************************
   Function: PosInLine
   Description:
     Checks out whether a position falls in a line.
*/
static BOOLEAN PosInLine(int nPos, int nLen, int nDirection)
{
  if (nDirection == -1)  /* Direction backward */
    if (nPos == 0)  /* There's no way in this direction */
      return FALSE;
  if (nPos >= 0 && nPos < nLen)
    return TRUE;
  else
    return FALSE;
}

/* ************************************************************************
   Function: IsAlpha
   Description:
     Checks whether a character is alpha:
     A-Z, a-z, 0-9, 128-254, _
     and sSuplSet (supplementary set)
*/
static BOOLEAN InAlpha(char c, char *sSuplSet)
{
  char *p;

  if (c >= 'A' && c <= 'Z')
    return TRUE;
  if (c >= 'a' && c <= 'z')
    return TRUE;
  if (c >= '0' && c <= '9')
    return TRUE;
  if ((unsigned char)c >= 128 && (unsigned char)c <= 254)
    return TRUE;
  if (c == '_')
    return TRUE;

  p = sSuplSet;
  if (p)
    while (*p)
      if (*p++ == c)
        return TRUE;

  return FALSE;
}

/* ************************************************************************
   Function: GotoWord
   Description:
     Where to positionize into a string moving on Direction
     Pos in 0..Length
     Direction = 1 -- forward, -1 -- backward
     bGotoEndOfWord = if cursor is already on a word, moving forward
       and this flag is set, then it moves to the edge of the current word
*/
void GotoWord(char *sLine, int nLnLen, int *nPos, int nDirection, BOOLEAN *bGotoLine,
  char *sSuplSet, BOOLEAN bGotoEndOfWord)
{
  int CountSpaces;
  int CountAlphas;

  *bGotoLine = FALSE;
  if (nDirection == 1)
  {
    /* When forward check whether we'are outside the line */
    if (*nPos >= nLnLen)
    {
      /* Position exceeds the last line position */
      SetGotoLine:
      *bGotoLine = TRUE;
      *nPos = 0;
      return;
    }
  }
  else /* Direction backward */
    if (*nPos == 0)  /* Position to go to is on the previous line */
      goto SetGotoLine;

  CountSpaces = 0;
  if (!InAlpha(sLine[*nPos], sSuplSet))
  {
    /* Find first alpha character */
    while (PosInLine(*nPos, nLnLen, nDirection) && !InAlpha(sLine[*nPos], sSuplSet))
    {
      *nPos += nDirection;
      CountSpaces++;
    }
    if (nDirection == 1)  /* First alpha character of next word */
      return;
    if (*nPos == 0)  /* And direction backward */
      goto SetGotoLine;
  }

  /* Go to edge of the word */
  CountAlphas = 0;
  while (PosInLine(*nPos, nLnLen, nDirection) &&
    InAlpha(sLine[*nPos + nDirection], sSuplSet))
  {
    *nPos += nDirection;
    CountAlphas++;
  }

  if (nDirection == -1)  /* Beginnig of current word */
  {
    if (CountAlphas != 0 || CountSpaces != 0)
      return;
    else
      if (*nPos == 0)
        goto SetGotoLine;
  }
  else
    if (bGotoEndOfWord)
    {
      if (CountAlphas != 0 || InAlpha(sLine[*nPos], sSuplSet))
      {
        *nPos += nDirection;
        return;
      }
    }

  /* Goto the edge of the next word */
  *nPos += nDirection;
  /* Jumpt through the white spaces */
  while (PosInLine(*nPos, nLnLen, nDirection) &&
    !InAlpha(sLine[*nPos], sSuplSet) && *nPos != 0)
  {
    *nPos += nDirection;
  }

  if (nDirection == -1) /* Goto the start of the word */
  {
    if (*nPos == 0)
      goto SetGotoLine;
    else
      while (PosInLine(*nPos, nLnLen, nDirection) &&
	InAlpha(sLine[*nPos + nDirection], sSuplSet)
        && *nPos != 0)
      {
        *nPos += nDirection;
      }
  }
}

/* ************************************************************************
   Function: GetWordUnderCursor
   Description:
     Gets the word that is under the current cursor position.
     psDest receives a copy of the word content.
*/
void GetWordUnderCursor(const TFile *pFile, char *psDest, int nDestBufSize)
{
  char *pLine;
  char *p;
  char *d;

  ASSERT(psDest != NULL);
  ASSERT(nDestBufSize > 0);

  *psDest = '\0';  /* Empty by default */

  if (pFile->pCurPos == NULL)
    return;  /* At <*** End Of File ***> position */

  if (!InAlpha(*pFile->pCurPos, NULL))
    return;  /* No word under cursor */

  p = pFile->pCurPos;
  pLine = GetLineText(pFile, pFile->nRow);
  /* Goto the start of the word */
  while (p > pLine && InAlpha(*(p - 1), NULL))
    --p;

  /* Copy the word */
  d = psDest;
  --nDestBufSize;
  while (*p && InAlpha(*p, NULL) && d - psDest < nDestBufSize)
    *d++ = *p++;

  *d = '\0';  /* Make the destination ASCIIZ */
}

/* ************************************************************************
   Function: CurPosInAlpha
   Description:
     Checks whether at the current cursor position there's a
     character InAlpha.
*/
BOOLEAN CurPosInAlpha(char *pCurPos, char *sSuplSet)
{
  if (pCurPos == NULL)  /* No character at all */
    return FALSE;

  return InAlpha(*pCurPos, sSuplSet);
}

/* ************************************************************************
   Function: FindPos
   Description:
     Finds the position in a line that is relevant to column cursor
     position.

     If the cursor position falls in a tab field:
     bTabLeft if set will order pFile->nCol to get the tab position.
     bCursorThroughTabs	if set will preserve pFile->Col even inside
     a tab field.

     On exit:
     pFile->pCurPos will point to a position required by pFile->nCol.
     pFile->nCol will jump over a tab ('\t') depending on options.
*/
void FindPos(TFile *pFile)
{
  char *p;
  int nNewPos;
  int nPrevPos;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->pCurPos == NULL)
    return;  /* The file is empty or we are at the <*** EndOfFile ***> line */

  /* Get the text component of the line */
  p = GetLineText(pFile, pFile->nRow);

  nNewPos = 0;
  nPrevPos = 0;
  while (*p)
  {
    if (nNewPos >= pFile->nCol)  /* We get to the desired column */
      break;
    nPrevPos = nNewPos;
    if (*p == '\t')  /* Tab char */
      nNewPos = CalcTab(nNewPos);
    else
      nNewPos++;
    p++;
  }

  if (nNewPos > pFile->nCol)
  {
    /*
    Inside tab area
    */
    if (bCursorThroughTabs)
      --p;  /* Position at the tab character */
    else
    {
      /* Here is maintained the efect of leaping over the tab characters */
      if (bTabLeft)
      {
        pFile->nCol = nPrevPos;  /* Jump at the start of the tab field */
        p--;  /* Let cur pos be exactly at the tab character */
      }
      else
        pFile->nCol = nNewPos;  /* Jump at the end of the tab field */
    }
  }

  pFile->pCurPos = p;
}

/* ************************************************************************
   Function: GotoColRow
   Description:
     Changes current cursor position.
*/
void GotoColRow(TFile *pFile, int nCol, int nRow)
{
  ASSERT(VALID_PFILE(pFile));

  bTabLeft = FALSE;
  if (pFile->nCol == nCol + 1 && pFile->nRow == nRow) /* We move toward left */
    bTabLeft = TRUE;

  if (nCol >= 0)
    pFile->nCol = nCol;

  if (nRow >= pFile->nNumberOfLines)
  {
    pFile->nRow = pFile->nNumberOfLines;
    pFile->pCurPos = NULL;
  }
  else
    if (nRow >= 0)
    {
      pFile->nRow = nRow;
      pFile->pCurPos = GetLineText(pFile, nRow);  /* Only indicate there's text */
    }

  if (pFile->pCurPos != NULL)
    FindPos(pFile);
}

/* ************************************************************************
   Function: GotoPosRow
   Description:
     Changes the current cursor position. Differs with GotoColRow()
     by processing nPos, which is a position in the text string
     of	a line, instead of column position.
   Example:
     Let have "abc\tfrt" as text of line #7. To positionize
     the cursor at the position of character 'f', call
     GotoPosRow(pFile, 5, 7). As a result the current column position
     will take on consideration that a \t (tab) character is present
     and will be 9 (if nTabSize is 8).
*/
void GotoPosRow(TFile *pFile, int nPos, int nRow)
{
  char *pCurLine;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nRow < pFile->nNumberOfLines);

  pFile->nRow = nRow;
  pCurLine = GetLineText(pFile, nRow);

  ASSERT(nPos <= (int)strlen(pCurLine));

  pFile->pCurPos = pCurLine + nPos;
  pFile->nCol = GetTabPos(pFile, nRow, pFile->pCurPos - GetLineText(pFile, nRow));
}

/* ************************************************************************
   Function: GotoPage
   Description:
     Moves the cursor one page up or down. The cursor position
     relatively to the screen remains the same.
*/
void GotoPage(TFile *pFile, int nWinHeight, BOOLEAN bPageUp)
{
  int nOldPage;
  int nJumpHeight;
  int nNewPageLine;

  nOldPage = pFile->nTopLine;
  nJumpHeight = nWinHeight - 2;
  if (bPageUp)
    nJumpHeight = -nJumpHeight;
  nNewPageLine = pFile->nTopLine + nJumpHeight;
  if (nNewPageLine < 0)  /* In the beginning of the 1st page */
    GotoColRow(pFile, pFile->nCol, 0);
  else
  {
    if (nNewPageLine > pFile->nNumberOfLines)
    {
      /* Goto the last line of the file */
      nJumpHeight = pFile->nNumberOfLines - pFile->nRow;
      pFile->nTopLine += nJumpHeight;
    }
    else  /* 0 <= NewPageLine <= AFile->nNumberOfLines */
      pFile->nTopLine = nNewPageLine;
    GotoColRow(pFile, pFile->nCol, pFile->nRow + nJumpHeight);
  }
  if (nOldPage != pFile->nTopLine)
    pFile->bUpdatePage = TRUE;
}

/* ************************************************************************
   Function: LeapThroughSelection
   Description:
     This function shoule be called only in non-persistent block
     mode to move the cursors over text selection.
*/
void LeapThroughSelection(TFile *pFile, BOOLEAN bForward)
{
  int nPos;

  ASSERT(VALID_PFILE(pFile));

  if (!pFile->bBlock)
    return;

  if (bBlockMarkMode)
    return;

  if (bForward)
  {
    /*
    Jump at the end of the selection if the cursor is
    at the start of the block.
    */
    if (pFile->blockattr & COLUMN_BLOCK)
    {
      if (pFile->nRow == pFile->nStartLine &&
        pFile->nCol == pFile->nStartPos)
        GotoColRow(pFile, pFile->nEndPos, pFile->nEndLine);
    }
    else
    {
      if (pFile->pCurPos == NULL)
        nPos = 0;
      else
        nPos = INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos);
      if (pFile->nRow == pFile->nStartLine &&
        nPos == pFile->nStartPos)
      {
        if (pFile->nEndLine == pFile->nNumberOfLines)
          GotoColRow(pFile, 0, pFile->nEndLine);
        else
          GotoPosRow(pFile, pFile->nEndPos + 1, pFile->nEndLine);
      }
    }
    return;
  }

  /*
  Jump at the start of the selection if the cursor is
  at the end of the block.
  */
  if (pFile->blockattr & COLUMN_BLOCK)
  {
    if (pFile->nRow == pFile->nEndLine &&
      pFile->nCol == pFile->nEndPos)
      GotoColRow(pFile, pFile->nStartPos, pFile->nStartLine);
  }
  else
  {
    if (pFile->pCurPos == NULL)
      nPos = -1;
    else
      nPos = INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos) - 1;
    if (pFile->nRow == pFile->nEndLine &&
      nPos == pFile->nEndPos)
      GotoPosRow(pFile, pFile->nStartPos, pFile->nStartLine);
  }
}

/* ************************************************************************
   Function: CharLeft
   Description:
*/
void CharLeft(TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));
  GotoColRow(pFile, pFile->nCol - 1, pFile->nRow);
}

/* ************************************************************************
   Function: CharRight
   Description:
*/
void CharRight(TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));
  GotoColRow(pFile, pFile->nCol + 1, pFile->nRow);
}

/* ************************************************************************
   Function: LineUp
   Description:
*/
void LineUp(TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));
  GotoColRow(pFile, pFile->nCol, pFile->nRow - 1);
}

/* ************************************************************************
   Function: LineDown
   Description:
*/
void LineDown(TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));
  GotoColRow(pFile, pFile->nCol, pFile->nRow + 1);
}

/* ************************************************************************
   Function: GotoNextWord
   Description:
*/
void GotoNextWord(TFile *pFile)
{
  char *pCurLine;
  BOOLEAN bGotoLine;
  int nPos;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->pCurPos == NULL)
    return;  /* We are at the <***End Of File***> line or file is empty */

_FindLoop:
  pCurLine = GetLineText(pFile, pFile->nRow);
  nPos = pFile->pCurPos - pCurLine;
  GotoWord(pCurLine, strlen(pCurLine), &nPos, 1, &bGotoLine, NULL, FALSE);

  if (bGotoLine)
  {
    if (pFile->nRow == pFile->nNumberOfLines - 1)
      return;  /* No next line: exit now */
    GotoColRow(pFile, 0, pFile->nRow + 1);
    if (!CurPosInAlpha(pFile->pCurPos, NULL))
      goto _FindLoop;
  }
  else
    GotoPosRow(pFile, nPos, pFile->nRow);  /* Move in the range of the current line */
}

/* ************************************************************************
   Function: GotoNextWordOrWordEnd
   Description:
     This is called to manage ctrl+arrow-right. It must move to the edge
     of the current word if cursor is over a word already, otherwis
     to move the beginning of the next word.
*/
void GotoNextWordOrWordEnd(TFile *pFile)
{
  char *pCurLine;
  BOOLEAN bGotoLine;
  int nPos;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->pCurPos == NULL)
    return;  /* We are at the <***End Of File***> line or file is empty */

  if (CurPosInAlpha(pFile->pCurPos, NULL))
  {
    pCurLine = GetLineText(pFile, pFile->nRow);
    nPos = pFile->pCurPos - pCurLine;
    GotoWord(pCurLine, strlen(pCurLine), &nPos, 1, &bGotoLine, NULL, TRUE);
    GotoPosRow(pFile, nPos, pFile->nRow);  /* Move in the range of the current line */
    return;
  }

  GotoNextWord(pFile);
}

/* ************************************************************************
   Function: GotoPrevWord
   Description:
*/
void GotoPrevWord(TFile *pFile)
{
  BOOLEAN bGotoLine;
  int nPos;
  char *pPrevLine;
  char *pCurLine;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->pCurPos == NULL)
  {
    if (pFile->nRow == 0)
      return;  /* File is empty */
    /* We are at the <***End Of File***> line */
_LastChar:
    pPrevLine = GetLineText(pFile, pFile->nRow - 1);
    GotoPosRow(pFile, strlen(pPrevLine), pFile->nRow - 1);
    return;
  }

  pCurLine = GetLineText(pFile, pFile->nRow);
  nPos = pFile->pCurPos - pCurLine;
  GotoWord(pCurLine, strlen(pCurLine), &nPos, -1, &bGotoLine, NULL, FALSE);

  if (bGotoLine)
  {
    if (pFile->nRow > 0)
      goto _LastChar;
  }
  else
    GotoPosRow(pFile, nPos, pFile->nRow);  /* Move in the range of the current line */
}

/* ************************************************************************
   Function: GotoHomePosition
   Description:
*/
void GotoHomePosition(TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));
  GotoColRow(pFile, 0, pFile->nRow);
}

/* ************************************************************************
   Function: GotoEndPosition
   Description:
*/
void GotoEndPosition(TFile *pFile)
{
  int nPos;
  char *pLine;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->pCurPos == NULL)
    return;
  pLine = GetLineText(pFile, pFile->nRow);
  nPos = strchr(pLine, '\0') - pLine;
  GotoPosRow(pFile, nPos, pFile->nRow);
}

/* ************************************************************************
   Function: GotoTop
   Description:
*/
void GotoTop(TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));
  GotoColRow(pFile, pFile->nCol, 0);
}

/* ************************************************************************
   Function: GotoBottom
   Description:
*/
void GotoBottom(TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));
  GotoColRow(pFile, pFile->nCol, pFile->nNumberOfLines);
}

/* ************************************************************************
   Function: GotoPageUp
   Description:
*/
void GotoPageUp(TFile *pFile, int nPageSize)
{
  ASSERT(VALID_PFILE(pFile));
  GotoPage(pFile, nPageSize, TRUE);
}

/* ************************************************************************
   Function: GotoPageDown
   Description:
*/
void GotoPageDown(TFile *pFile, int nPageSize)
{
  ASSERT(VALID_PFILE(pFile));
  GotoPage(pFile, nPageSize, FALSE);
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

