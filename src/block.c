/*

File: block.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 31st March, 1999
Descrition:
  Functions concerning text block storage and manipulation

*/

#include "global.h"
#include "memory.h"
#include "file.h"
#include "wline.h"
#include "nav.h"
#include "l1opt.h"
#include "undo.h"
#include "bookm.h"
#include "synh.h"
#include "edinterf.h"
#include "block.h"

/* ************************************************************************
   Function: AllocTBlock
   Description:
*/
TBlock *AllocTBlock(void)
{
  TBlock *pBlock;

  pBlock = alloc(sizeof(TBlock));
  #ifdef _DEBUG
  if (pBlock != NULL)
    pBlock->MagicByte = BLOCK_MAGIC;
  #endif

  return pBlock;
}

/* ************************************************************************
   Function: FreeTBlock
   Description:
*/
void FreeTBlock(TBlock *pBlock)
{
  ASSERT(VALID_PBLOCK(pBlock));

  #ifdef _DEBUG
  pBlock->MagicByte = 0;
  #endif
  s_free(pBlock);
}

/* ************************************************************************
   function: tblock_copy_to_str
   description:
     creates an asciiz string representation of a block in the heap
   returns:
     1. _dest_ hold the block formatted as a string, function
     returns the address passed as dest
     -or-
     2. if dest_buf_size is smaller than the space needed it returns a
     pointer to the copy (xmalloc) in the heap, caller must call xfree()
*/
char *tblock_copy_to_str(TBlock *pblock, int dest_eol_type,
  char *dest, int dest_buf_size)
{
  int dest_size;
  int eol_size;
  char *pdest;
  char *p;
  int i;

  /*
  Calculate block size
  */
  dest_size = 0;
  eol_size = (dest_eol_type == CRLFtype ? 2 : 1);
  for (i = 0; i < pblock->nNumberOfLines; ++i)
    dest_size += pblock->pIndex[i].nLen + eol_size;

  /*
  Allocate global memory to compose the clipboard text
  */
  if (dest_size + 1 > dest_buf_size)
  {
    pdest = alloc(dest_size + 1);
    if (pdest == NULL)
      goto _exit;
  }
  else
    pdest = dest;

  p = pdest;
  for (i = 0; i < pblock->nNumberOfLines; ++i)
  {
    strcpy(p, pblock->pIndex[i].pLine);
    p = strchr(p, '\0');
    ASSERT(p != NULL);
    if (i != pblock->nNumberOfLines - 1)
    {
      switch (dest_eol_type)
      {
        case CRtype:
          *p++ = '\r';
          break;
        case LFtype:
          *p++ = '\n';
          break;
        case CRLFtype:
          *p++ = '\r';
          *p++ = '\n';
          break;
        default:
          ASSERT(0);  /* bad dest_eol_type */
      }
    }
  }
  *p = '\0';

_exit:
  return pdest;
}

/* ************************************************************************
   Function: MakeACopyOfBlock
   Description:
     Allocates a copy of a block from the current file in the heap.
*/
TBlock *MakeACopyOfBlock(const TFile *pFile,
  int nStartLine, int nEndLine, int nStartPos, int nEndPos,
  WORD blockattr)
{
  int nLine;
  int nBlockSize;
  int nEOLSize;
  char *p;
  const char *pLine;
  TLine *pLn;
  int nToCopy;
  TBlock *pBlock;
  int nLen;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nStartLine >= 0);
  ASSERT(nEndLine >= 0);
  ASSERT(nStartLine <= pFile->nNumberOfLines);
  ASSERT(nEndLine <= pFile->nNumberOfLines);
  ASSERT(nEndLine >= nStartLine);

  pBlock = AllocTBlock();
  if (pBlock == NULL)
    return NULL;

  if (blockattr & COLUMN_BLOCK)
  {
    ASSERT(nStartPos >= 0);
    ASSERT(nEndPos >= 0);
    ASSERT(nEndPos >= nStartPos);

    /*
    Calc the amount of space neccessary for the block to
    be reproduced.
    */
    nBlockSize = 0;  /* amount here */
    nEOLSize = (pFile->nEOLType == CRLFtype ? 2 : 1);
    pBlock->nEOLType = pFile->nEOLType;
    for (nLine = nStartLine; nLine <= nEndLine; ++nLine)
    {
      nLen = GetLine(pFile, nLine)->nLen;
      if (nLen > nEndPos)
        nBlockSize += nEndPos - nStartPos + 1;
      else
        if (nLen > nStartPos)
          nBlockSize += nLen - nStartPos;
        else
          ;  /* Only one empty piece line will be added as the whole line is outside */
      nBlockSize += nEOLSize;
    }

    /*
    Allocated a single block to put all the lines from the text block from
    the file.
    */
    pBlock->pBlock = AllocateTBlock(nBlockSize);
    if (pBlock->pBlock == NULL)
    {
_dispose_pBlock:
      FreeTBlock(pBlock);
      return NULL;
    }

    pBlock->blockattr = COLUMN_BLOCK;
    pBlock->nNumberOfLines = nEndLine - nStartLine + 1;
    TArrayInit(pBlock->pIndex, pBlock->nNumberOfLines, 1);
    if (pBlock->pIndex == NULL)
    {
      DisposeBlock(pBlock->pBlock);
      goto _dispose_pBlock;
    }
    pLn = pBlock->pIndex;
    p = pBlock->pBlock;

    /*
    Extract the text block from the file
    */
    for (nLine = nStartLine; nLine <= nEndLine; ++nLine)
    {
      pLine = GetLineText(pFile, nLine);

      /*
      Determine what is the particular part of this line (nLine)
      to be extracted.
      */
      nLen = GetLine(pFile, nLine)->nLen;
      if (nLen > nEndPos)
        nToCopy = nEndPos - nStartPos + 1;
      else
        if (nLen > nStartPos)
          nToCopy = nLen - nStartPos;
        else
          nToCopy = 0;  /* Whole line is left to the left edge of the column block */

      if (nToCopy > 0)
        strncpy(p, &pLine[nStartPos], nToCopy);
      p[nToCopy] = '\0';

      /*
      Add a raference in pIndex to the added line
      */
      pLn->pLine = p;
      pLn->pFileBlock = pBlock->pBlock;
      pLn->attr = 0;

      p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
      pLn->nLen = p - pLn->pLine;  /* Calc the line length */
      p += nEOLSize;

      ++pLn;
      ASSERT(p != NULL);
    }

    IncRef(pBlock->pBlock, pBlock->nNumberOfLines);  /* Update the file block reference counter */
    TArraySetCount(pBlock->pIndex, pBlock->nNumberOfLines);
  }
  else
  {
    ASSERT(nStartPos >= 0);
    ASSERT(nEndPos >= -1);
    /*
    Q: When nEndPos could be set to -1?
    A: In the case when the block starts at a line and ends at the
    very start of the next line but not containing even the first
    character of the line.
    */

    /*
    Calc the amount of space neccessary for the block to
    be reproduced.
    */
    nBlockSize = 0;  /* amount here */
    nEOLSize = (pFile->nEOLType == CRLFtype ? 2 : 1);
    pBlock->nEOLType = pFile->nEOLType;
    for (nLine = nStartLine; nLine <= nEndLine; ++nLine)
    {
      /*
      The 3 possible cases of block in a line are described
      bellow in the loop of extracting the block.
      */
      nBlockSize += nEOLSize;
      if (nLine == pFile->nNumberOfLines)
        continue;
      pLn = GetLine(pFile, nLine);
      if (nLine == nStartLine)
      {
        if (nLine == nEndLine)
          nBlockSize += nEndPos - nStartPos + 1;
        else
          nBlockSize += pLn->nLen - nStartPos;
      }
      else
        if (nLine == nEndLine)
          nBlockSize += nEndPos + 1;
        else
          nBlockSize += pLn->nLen;
    }

    /*
    Allocated a single block to put all the lines from the text block from
    the file.
    */
    pBlock->pBlock = AllocateTBlock(nBlockSize);
    if (pBlock->pBlock == NULL)
      goto _dispose_pBlock;
    memset(pBlock->pBlock, '\0', nBlockSize);

    pBlock->blockattr = 0;  /* !COLUMN_BLOCK */
    pBlock->nNumberOfLines = nEndLine - nStartLine + 1;
    TArrayInit(pBlock->pIndex, pBlock->nNumberOfLines, 1);
    if (pBlock->pIndex == NULL)
    {
      DisposeBlock(pBlock->pBlock);
      goto _dispose_pBlock;
    }
    pLn = pBlock->pIndex;
    p = pBlock->pBlock;

    /*
    Extract the text block from the file
    */
    for (nLine = nStartLine; nLine <= nEndLine; ++nLine)
    {
      if (nLine == pFile->nNumberOfLines)
        goto _put_ref;  /* Empty line for <*** End Of File ***> */

      pLine = GetLineText(pFile, nLine);

      /*
      there are 4 cases of a line in a block.
      1. The line is at the start of the block (nLn == nStartLine).
        - Copy the line from nStatPos up to end.
      2. The line is at the end of the block (nLn == nEndLine).
        - Cut the line copy at nEndPos.
      3. The line contains all the block (nLn == nStartLine && nLn == nEndLine).
        - Extract a chunk from in the middle of the line nStartPos..nEndPos.
      4. This is a whole line inside the block
        - Copy the whole line
      */
      if (nLine == nStartLine)
      {
        ASSERT(nStartPos <= (int)strlen(pLine));

        if (nLine != nEndLine)  /* case 1. */
          strcpy(p, &pLine[nStartPos]);  /* from nStartPos up to the end of the line */
        else  /* case 3. */
        {
          ASSERT(nEndPos <= (int)strlen(pLine));
          ASSERT(nEndPos >= nStartPos);

          nToCopy = nEndPos - nStartPos + 1;
          strncpy(p, &pLine[nStartPos], nToCopy);  /* a piece in the middle */
          p[nToCopy] = '\0';
        }
      }
      else
        if (nLine == nEndLine)
        {
          /* case 2. */
          nToCopy = nEndPos + 1;
          strncpy(p, pLine, nToCopy);
          p[nToCopy] = '\0';
        }
        else  /* case 4. */
          strcpy(p, pLine);

_put_ref:
      /*
      Add a raference in pIndex to the added line
      */
      pLn->pLine = p;
      pLn->pFileBlock = pBlock->pBlock;
      pLn->attr = 0;

      p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
      pLn->nLen = p - pLn->pLine;  /* Calc the line length */
      p += nEOLSize;

      ++pLn;
      ASSERT(p != NULL);
    }
    ASSERT(p - pBlock->pBlock == nBlockSize);

    IncRef(pBlock->pBlock, pBlock->nNumberOfLines);  /* Update the file block reference counter */
    TArraySetCount(pBlock->pIndex, pBlock->nNumberOfLines);
  }

  return pBlock;
}

/* ************************************************************************
   Function: MakeACopyOfTheBlock
   Description:
     Makes a copy of a block in the current file
*/
static TBlock *MakeACopyOfTheBlock(const TFile *pFile)
{
  if (!pFile->bBlock)
    return NULL;

  return MakeACopyOfBlock(pFile, pFile->nStartLine, pFile->nEndLine,
    pFile->nStartPos, pFile->nEndPos, pFile->blockattr);
}

/* ************************************************************************
   Function: GetBlockLine
   Description:
*/
TLine *GetBlockLine(const TBlock *pBlock, int nLine)
{
  ASSERT(VALID_PBLOCK(pBlock));
  ASSERT(VALID_PARRAY(pBlock->pIndex));
  ASSERT(nLine >= 0);
  ASSERT(nLine <= pBlock->nNumberOfLines);

  return &pBlock->pIndex[nLine];
}

/* ************************************************************************
   Function: DuplicateBlock
   Description:
     Makes an exact copy of a block in the heap.
     Converts the new block copy to the specified EOL type.
     Ensures nPrefixSize and nSuffixSize characters respectively
     before and after the actual block copy.
     On exit:
     *nSuffixPos will hold the position where the suffix
     part should be copied.
     *pnLastLineLen will hold the length of the last block line

*/
TBlock *DuplicateBlock(const TBlock *pBlock1, int nNewEOLType,
  int nPrefixSize, int nSuffixSize, int *pnSuffixPos, int *pnLastLineLen)
{
  TBlock *pBlock2;
  int nBlockSize;
  int nLine;
  TLine *pLn;
  char *p;
  int nEOLSize;
  const char *pLine;

  ASSERT(pBlock1 != NULL);

  pBlock2 = AllocTBlock();
  if (pBlock2 == NULL)
    return NULL;

  /*
  Calc the amount of space neccessary for the block to
  be reproduced.
  */
  nBlockSize = 0;  /* amount here */
  nEOLSize = (nNewEOLType == CRLFtype ? 2 : 1);
  for (nLine = 0; nLine < pBlock1->nNumberOfLines; ++nLine)
    nBlockSize += GetBlockLine(pBlock1, nLine)->nLen + nEOLSize;

  /*
  Allocated a single block to put all the lines from the text block from
  the file.
  */
  pBlock2->pBlock = AllocateTBlock(nBlockSize + nPrefixSize + nSuffixSize);
  if (pBlock2->pBlock == NULL)
  {
_dispose_pBlock:
    FreeTBlock(pBlock2);
    return NULL;
  }

  pBlock2->nEOLType = nNewEOLType;
  pBlock2->nNumberOfLines = pBlock1->nNumberOfLines;
  pBlock2->blockattr = pBlock1->blockattr;
  TArrayInit(pBlock2->pIndex, pBlock2->nNumberOfLines, 1);
  if (pBlock2->pIndex == NULL)
  {
    DisposeBlock(pBlock2->pBlock);
    goto _dispose_pBlock;
  }
  pLn = pBlock2->pIndex;
  p = pBlock2->pBlock + nPrefixSize;

  /*
  Extract the text block from the file
  */
  for (nLine = 0; nLine < pBlock1->nNumberOfLines; ++nLine)
  {
    pLine = GetBlockLineText(pBlock1, nLine);
    strcpy(p, pLine);

    /*
    Add a raference in pIndex to the added line
    */
    pLn->pLine = p;
    pLn->pFileBlock = pBlock2->pBlock;  /* Store the block where _p_ refers to */
    pLn->attr = 0;

    p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
    pLn->nLen = p - pLn->pLine;  /* Calc the line length */
    if (pnLastLineLen != NULL)
      *pnLastLineLen = pLn->nLen;
    p += nEOLSize;

    ++pLn;
    ASSERT(p != NULL);
  }

  IncRef(pBlock2->pBlock, pBlock2->nNumberOfLines);  /* Update the file block reference counter */
  TArraySetCount(pBlock2->pIndex, pBlock2->nNumberOfLines);

  /*
  Calc the position where the suffix part should be copied.
  Now _p_ contains the end position of the last line.
  Text copied at *pnSuffixPos would be concatenated to the end of the
  last line.
  */
  if (pnSuffixPos != NULL)
    *pnSuffixPos = p - pBlock2->pBlock - nEOLSize;

  return pBlock2;
}

/* ************************************************************************
   Function: DisposeABlock
   Description:
*/
void DisposeABlock(TBlock **pBlock)
{
  ASSERT(pBlock != NULL);
  ASSERT(VALID_PBLOCK(*pBlock));

  DisposeBlock((*pBlock)->pBlock);
  if ((*pBlock)->pIndex != NULL)
    TArrayDispose((*pBlock)->pIndex);

  FreeTBlock(*pBlock);
  *pBlock = NULL;
}

/* ************************************************************************
   Function: UpdateFunctionNamesBookmarks
   Description:
     This is called from UpdateMarkers functions of block.c
     Rescans from nRow, the next nNumberOfLines for changes:
     1. function name was edited
     2. new functions were inserted
     3. functions were deleted (those are removed automaticly by BMListUpdate)
     Function is called after BMListUpdate().
*/
static void UpdateFunctionNamesBookmarks(TFile *pFile, int nRow, int nNumberOfLines)
{
  int nLine;
  int nCalcRow;
  TMarkLocation *pMark;

  /* File's not part of the workspace? */
  if (pFile->pBMFuncFindCtx == NULL)
    return;

  if (nNumberOfLines < 0)  /* delete? */
  {
    nNumberOfLines = -nNumberOfLines;
    if (nNumberOfLines > 0)  /* check only for same line editings */
      return;
  }

  if (!BMSetFindCtxGetActiveFlag(pFile->pBMFuncFindCtx))
    return;

  /*
  For all lines nRow..nLastRow scan for any line having a function name
  bookmark
  */
  nLine = nRow;
  do
  {
    BMSetRevertToFirst(pFile->pBMFuncFindCtx);  /* not very expensive */
    BMSetNewSearchRow(pFile->pBMFuncFindCtx, nLine);
_next_mark:
    pMark = BMSetFindNextBookmark(pFile->pBMFuncFindCtx, &nCalcRow);

    if (pMark == NULL)
      break;

    if (nCalcRow != nLine)
    {
      ++nLine;
      continue;  /* No bookmarks for this specific line */
    }

    /*
    Filter out the not BMSetFuncNames bookmarks
    */
    if (pMark->pSet != pFile->pBMSetFuncNames)
      goto _next_mark;  /* get next mark, possibly for the same line */

    pFile->bUpdatePage = TRUE;  /* This will rescan the func names for this page */
    break;
  }
  while (nLine < nRow + nNumberOfLines);
}

/* ************************************************************************
   Function: InvalidateEOLStatus
   Description:
     Syntax highlighting needs to keep a state WORD for the status at
     end of every line. Status flows from the end of the first line to the
     very end of the last line. Inserting or deleting text in a line may lead
     to a change of the status at the end of the line, if this happens we have
     to reset the status to unset from this line to the end of file.
*/
static void InvalidateEOLStatus(TFile *pFile, int nStartLine)
{
  int i;
  TLine *pLine;

  ASSERT(nStartLine <= pFile->nNumberOfLines);

  for (i = nStartLine; i < pFile->nNumberOfLines; ++i)
  {
    pLine = GetLine(pFile, i);
    pLine->attr = 0;
  }
  pFile->bUpdatePage = TRUE;  /*  strictly speaking we need from current line
                                 +until the end of the page, instead of
                                 +redrawing the whole page */
}

/* ************************************************************************
   Function: UpdateMarkers
   Description:
     Called whenever a block of characters is inserted into a file.
     Updates current cursor positions markers.
     TODO: Updates all the row related markers in the file.

   On entry:
     pFile -- update here
     nRow -- the row where the characters block was inserted
     nCPosIndex -- index of the characters position where the block was
       inserted
     nNumberOfLines -- how much lines comprises the block
     nLastLineLen -- the length of the last line of the block
*/
static void UpdateMarkers(TFile *pFile, int nRow, int nCPosIndex,
  int nNumberOfLines, int nLastLineLen)
{
  /*
  Check to see if EOL status needs invalidation
  */
  if (GetEOLStatus(pFile, nRow) != pFile->lnattr)
    InvalidateEOLStatus(pFile, nRow);

  /*
  Update the cursor position.
  */
  if (nNumberOfLines == 1)
  {
    /* The whole inserted block is inside a single line */
    GotoPosRow(pFile, nCPosIndex + nLastLineLen, nRow);
  }
  else
  {
    /* A multi-line block was inserted */
    BMListUpdate(pFile, nRow, nNumberOfLines - 1);

    if (nRow + nNumberOfLines - 1 == pFile->nNumberOfLines)
      GotoColRow(pFile, 0, pFile->nNumberOfLines);  /* text inserted at end of file */
    else
      GotoPosRow(pFile, nLastLineLen, nRow + nNumberOfLines - 1);
  }

  /*
  Update the block markers.
  HINT: As character blocks nStartPos/nEndPos are indices of a particular
  character positions, so they could be compared against nCPosIndex.
  */
  if (nRow < pFile->nStartLine)
  {
    if (pFile->nStartLine != -1)  /* if defined only */
      pFile->nStartLine += nNumberOfLines - 1;
  }
  else
    if (nRow == pFile->nStartLine && (pFile->blockattr & COLUMN_BLOCK) == 0)
    {
      ASSERT(pFile->nStartPos >= 0);  /* nStartLine/nStartPos should be a valid pair */

      if (nCPosIndex < pFile->nStartPos)
        pFile->nStartPos += nLastLineLen;
    }

  if (nRow < pFile->nEndLine)
  {
    if (pFile->nEndLine != -1)  /* if defined only */
      pFile->nEndLine += nNumberOfLines - 1;
  }
  else
    if (nRow == pFile->nEndLine && (pFile->blockattr & COLUMN_BLOCK) == 0)
    {
      ASSERT(pFile->nEndPos >= -1);  /* nEndLine/nEndPos should be a valid pair */

      if (nCPosIndex <= pFile->nEndPos)
        pFile->nEndPos += nLastLineLen;
    }

  UpdateFunctionNamesBookmarks(pFile, nRow, nNumberOfLines);
}

/* ************************************************************************
   Function: InsertCharacterBlockPrim
   Description:
     Inserts a character block at the current cursor position in a file.
     In case of failing to insert pBlock (no enough memory), the
     function leaves the file in the exact same status as prior
     invoking it.
*/
BOOLEAN InsertCharacterBlockPrim(TFile *pFile, const TBlock *pBlock)
{
  int nPrefixSize;
  int nSuffixSize;
  char *pLine;
  int nLen;
  TBlock *pNewBlock;
  int nSuffixPos;
  int nOldLine;
  TLine *pOldLine;
  int nNumberOfLines;
  int nLastLineLen;
  BOOLEAN bInsertingSingleEmptyLine;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PBLOCK(pBlock));
  ASSERT(pBlock->nNumberOfLines >= 0);

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return FALSE;

  /*
  Check the current cursor position and examine the size
  of the two portions a line should be split to -- nPrefixSize, nSuffixSize
  */
  if (pFile->pCurPos == NULL)  /* Empty file or <*** end of file ***> line */
    nPrefixSize = nSuffixSize = 0;
  else
  {
    pLine = GetLineText(pFile, pFile->nRow);
    nLen = GetLine(pFile, pFile->nRow)->nLen;

    nPrefixSize = pFile->pCurPos - pLine;
    ASSERT(nPrefixSize >= 0);
    ASSERT(nPrefixSize <= nLen);

    nSuffixSize = nLen - nPrefixSize;
  }

  /*
  Create the new block that should be placed right inbetween
  the split line.
  */
  pNewBlock = DuplicateBlock(pBlock, pFile->nEOLType,
    nPrefixSize, nSuffixSize, &nSuffixPos, &nLastLineLen);
  if (pNewBlock == NULL)
    return FALSE;

  /*
  Copy the prefix and the suffix part of the line in pNewBlock
  */
  if (pFile->pCurPos != NULL)
  {
    strncpy(pNewBlock->pBlock, pLine, nPrefixSize);
    strcpy(pNewBlock->pBlock + nSuffixPos, pFile->pCurPos);
  }

  /*
  Update the lenghts of the lines that contain the prefix and the suffix.
  Update the pNewBlock->pLine with the size of the prefix.
  */
  nNumberOfLines = pNewBlock->nNumberOfLines;
  GetBlockLine(pNewBlock, 0)->nLen += nPrefixSize;
  GetBlockLine(pNewBlock, nNumberOfLines - 1)->nLen += nSuffixSize;
  GetBlockLine(pNewBlock, 0)->pLine -= nPrefixSize;

  /*
  Store the line status to be compared after the end of the operation
  */
  pFile->lnattr = GetEOLStatus(pFile, pFile->nRow);

  /*
  Insert all the new lines in the index array of the file.
  */
  if (pFile->pIndex == NULL)
  {
    /*
    Empty file -- prapare an Index
    */
    TArrayInit(pFile->pIndex, pFile->nNumberOfLines, FILE_DELTA);
    if (pFile->pIndex == NULL)
    {
_dispose_newblock:
      DisposeABlock(&pNewBlock);
      return FALSE;
    }
    /*
    Insert the block.
    pNewBlock->nNumberOfLines - 1 because last line is <*** end of file ***>
    */
    TArrayInsertGroup(pFile->pIndex, 0,
      pNewBlock->pIndex, pNewBlock->nNumberOfLines - 1);
    if (!TArrayStatus(pFile->pIndex))  /* Failed to insert the lines */
    {
      TArrayClearStatus(pFile->pIndex);
      goto _dispose_newblock;
    }
    pFile->nNumberOfLines = pNewBlock->nNumberOfLines - 1;
    AddBlockLink(&pFile->blist, pNewBlock->pBlock);
    goto _dispose_newtblock;
  }

  if (pBlock->nNumberOfLines == 2
      && GetBlockLine(pBlock, 0)->nLen == 0
      && GetBlockLine(pBlock, 1)->nLen == 0)
  {
    bInsertingSingleEmptyLine = TRUE;
  }
  else
  {
    bInsertingSingleEmptyLine = FALSE;
  }

  /* check if last line is empty, then there's nothing to join it to */
  if (!bInsertingSingleEmptyLine
      && GetBlockLine(pNewBlock, nNumberOfLines - 1)->nLen == 0
      && pFile->nRow == pFile->nNumberOfLines - 1)
    --nNumberOfLines;

  /*
  Special case when all the block is contained in a single line.
  */
  if (nNumberOfLines == 1 && pFile->pCurPos != NULL)
  {
    pOldLine = GetLine(pFile, pFile->nRow);
    DecRef(pOldLine->pFileBlock, 1);
    pFile->pIndex[pFile->nRow] = pNewBlock->pIndex[0];
    AddBlockLink(&pFile->blist, pNewBlock->pBlock);
    goto _dispose_newtblock;
  }

  /*
  Multi-line block.
  */
  if (pFile->pCurPos == NULL)  /* Inserting at EOF */
  {
    --nNumberOfLines;
    TArrayInsertGroup(pFile->pIndex, pFile->nRow,
      pNewBlock->pIndex, nNumberOfLines);
    if (!TArrayStatus(pFile->pIndex))  /* Failed to insert the lines */
    {
      TArrayClearStatus(pFile->pIndex);
      goto _dispose_newblock;
    }
    pFile->nNumberOfLines += nNumberOfLines;
    AddBlockLink(&pFile->blist, pNewBlock->pBlock);
    pFile->bUpdatePage = TRUE;  /* Screen update is necessary */
  }
  else
  {
    TArrayInsertGroup(pFile->pIndex, pFile->nRow,
      pNewBlock->pIndex, nNumberOfLines);
    if (!TArrayStatus(pFile->pIndex))  /* Failed to insert the lines */
    {
      TArrayClearStatus(pFile->pIndex);
      goto _dispose_newblock;
    }
    pFile->nNumberOfLines += nNumberOfLines - 1;
    AddBlockLink(&pFile->blist, pNewBlock->pBlock);
    /* Remove the original version of the line that was split */
    nOldLine = pFile->nRow + nNumberOfLines;  /* Old line position */
    pOldLine = GetLine(pFile, nOldLine);
    DecRef(pOldLine->pFileBlock, 1);  /* As this line is to be removed */
    TArrayDeleteGroup(pFile->pIndex, nOldLine, 1);
  }

  /*
  As pNewBlock->pIndex was copied and all the pBlock parameters
  are moved into pFile dispose the pBlock.
  */
_dispose_newtblock:
  TArrayDispose(pNewBlock->pIndex);
  FreeTBlock(pNewBlock);

  /*
  Update the cursor position to point at the end of the block.
  Update any row related markers.
  */
  UpdateMarkers(pFile, pFile->nRow, nPrefixSize, nNumberOfLines, nLastLineLen);

  if (nNumberOfLines == 1)
    pFile->bUpdateLine = TRUE;
  else
    pFile->bUpdatePage = TRUE;

  pFile->bUpdateStatus = TRUE;

  return TRUE;
}

/* ************************************************************************
   Function: CalcBlockEndPosition
   Description:
     Calcs the end coordinate of a block that is to be insertet
     at nStartLine/nStartPos.
*/
static void CalcBlockEndPosition(const TBlock *pBlock,
  int nStartLine, int nStartPos, int *pnEndLine, int *pnEndPos)
{
  ASSERT(VALID_PBLOCK(pBlock));
  ASSERT(pnEndLine != NULL);
  ASSERT(pnEndPos != NULL);
  ASSERT((pBlock->blockattr & COLUMN_BLOCK) == 0);

  *pnEndLine = nStartLine + pBlock->nNumberOfLines - 1;
  if (pBlock->nNumberOfLines == 1)
  {
    *pnEndPos = nStartPos + GetBlockLine(pBlock, 0)->nLen - 1;
    return;
  }
  *pnEndPos = GetBlockLine(pBlock, pBlock->nNumberOfLines - 1)->nLen - 1;
}

/* ************************************************************************
   Function: InsertCharacterBlock
   Description:
     This is a wrap of InsertCharacterBlockPrim().
     Adds undo logging of the operaion.
     Inserts at the current cursor position or uses
     nCol-nRow pair to set a position explicitely if specified.

     Q: Why not using GotoColRow() before calling InsertCharacterBlock()
     but instead passing as parameters the disired position?
     A: The difference is what will be stored as undo record for
     the cursor position. The explicit parameter nCol-nRow will
     be activated after AddUndoRecord().

   Parameters:
     nCol = -1 directs the function to use the current cursor position.
*/
static BOOLEAN InsertCharacterBlock(TFile *pFile, const TBlock *pBlock, int nCol, int nRow)
{
  int nUndoEntry;
  TBlock *pBlockCopy;
  BOOLEAN bResult;
  TLine *pLine;
  int nCurPos;
  int nEndLine;
  int nEndPos;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PBLOCK(pBlock));

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return FALSE;

  bResult = TRUE;

  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (!AddUndoRecord(pFile, acINSERT, FALSE))
  {
_failed1:
    bResult = FALSE;
    goto _exit_point;
  }

  if (nCol >= 0)
    GotoColRow(pFile, nCol, nRow);

  pBlockCopy = ExactBlockCopy(pBlock);
  if (pBlockCopy == NULL)
    goto _failed1;

  if (pFile->pCurPos == NULL)
    nCurPos = 0;  /* Inserting in empty file or at <*** End Of File ***> */
  else
  {
    pLine = GetLine(pFile, pFile->nRow);
    nCurPos = pFile->pCurPos - pLine->pLine;
    ASSERT(nCurPos <= pLine->nLen);
  }
  CalcBlockEndPosition(pBlock, pFile->nRow, nCurPos, &nEndLine, &nEndPos);
  RecordUndoData(pFile, pBlockCopy, pFile->nRow, nCurPos, nEndLine, nEndPos);

  bResult = InsertCharacterBlockPrim(pFile, pBlock);

  if (bResult)
  {
    pFile->bChanged = TRUE;
    pFile->bRecoveryStored = FALSE;
  }

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);
  return bResult;
}

#ifdef _DEBUG
/* ************************************************************************
   Function: CheckForTabPos
   Description:
     Checks for tab '\t' at the specified nCol:nRow position.
*/
static BOOLEAN CheckForTabPos(const TFile *pFile, int nRow, int nCol)
{
  const char *p;
  int nNewPos;
  int nPrevPos;

  p = GetLineText(pFile, nRow);

  nNewPos = 0;
  nPrevPos = 0;
  while (*p)
  {
    if (nNewPos >= nCol)  /* We get to the desired column */
      break;
    nPrevPos = nNewPos;
    if (*p == '\t')  /* Tab char */
      nNewPos = CalcTab(nNewPos);
    else
      nNewPos++;
    p++;
  }

  if (nNewPos > nCol)
    return TRUE;  /* '\t' at this position */
  return FALSE;  /* no tab at this position */
}
#else
#define CheckForTabPos(pFile, nRow, nCol) (1)
#endif

/* ************************************************************************
   Function: CalcBlanks
   Description:
     This function is altered by the options bUseTabs and bOptimalFill.
     Calculates how much blank spaces (' ' and '\t') are necessary
     to be filled particular area.
*/
static int CalcBlanks(int nStartCol, int nEndCol)
{
  int nCol;
  int nNextCol;
  int nBlanks;

  ASSERT(nStartCol < nEndCol);

  nCol = nStartCol;
  nBlanks = 0;  /* amount here */
  while (nCol < nEndCol)
  {
    nNextCol = CalcTab(nCol);
    if (nNextCol <= nEndCol && bOptimalFill && bUseTabs)
      nCol = nNextCol;  /* '\t' should be put at this position */
    else
      ++nCol;  /* ' ' should be put at this position */
    ++nBlanks;  /* one more tab or space */
  }

  return nBlanks;
}

/* ************************************************************************
   Function: PutBlanks
   Description:
     This function is altered by the options bUseTabs and bOptimalFill.
     Puts blank spaces (' ' and '\t') to fullfill a particular area.
*/
static void PutBlanks(char **pDest, int nStartCol, int nEndCol)
{
  int nCol;
  int nNextCol;

  ASSERT(pDest != NULL);
  ASSERT(*pDest != NULL);
  ASSERT(nStartCol < nEndCol);

  nCol = nStartCol;
  while (nCol < nEndCol)
  {
    nNextCol = CalcTab(nCol);
    if (nNextCol <= nEndCol && bOptimalFill && bUseTabs)
    {
      *(*pDest) = '\t';
      nCol = nNextCol;
    }
    else
    {
      *(*pDest) = ' ';
      ++nCol;  /* ' ' should be put at this position */
    }
    ++(*pDest);
  }
}

/* ************************************************************************
   Function: FitColumnBlock
   Description:
     Prepares column block so each of the lines to fit in their
     respective line positions.
     Starts at current cursor position.
*/
static TBlock *FitColumnBlock(const TFile *pFile, const TBlock *pBlock, int *pnBlockWidth)
{
  const TLine *pLine;
  const TLine *pBlockLine;
  TLine *pNewBlockLine;
  char nBlockLineLen;
  int nLastCol;
  int nBlockSize;
  TBlock *pNewBlock;
  int i;
  char *p;

  ASSERT((pBlock->blockattr & COLUMN_BLOCK) != 0);
  ASSERT(pnBlockWidth != NULL);

  /* Ensure that there are lines to fit all the column block lines */
  ASSERT(pFile->nRow + pBlock->nNumberOfLines <= pFile->nNumberOfLines);

  /*
  Calc column block width. This is the length of the longest line.
  */
  *pnBlockWidth = 0;
  for (i = 0; i < pBlock->nNumberOfLines; ++i)
  {
    if ((nBlockLineLen = GetBlockLine(pBlock, i)->nLen) > *pnBlockWidth)
      *pnBlockWidth = nBlockLineLen;
  }

  ASSERT(*pnBlockWidth > 0);

  /*
  Walk along the lines where each piece of line of the column block
  will be inserted and calc the necessary size of the block, that
  should be allocated to fit the transformed column block.
  */
  nBlockSize = 0;
  for (i = 0; i < pBlock->nNumberOfLines; ++i)
  {
    ASSERT(pFile->nRow + i < pFile->nNumberOfLines);

    pBlockLine = GetBlockLine(pBlock, i);
    pLine = GetLine(pFile, pFile->nRow + i);

    nBlockLineLen = pBlockLine->nLen;

    nLastCol = GetTabPos(pFile, pFile->nRow + i, pLine->nLen);

    if (nLastCol < pFile->nCol)
    {
      /*
      This line should be filled up with spaces to make up to the
      column block left edge position.
      The region to be filled is from column nLastCol up to pFile->nCol.
      */
      if (nBlockLineLen > 0)
        nBlockSize += CalcBlanks(nLastCol, pFile->nCol) + nBlockLineLen;
    }
    else
    {
      /*
      The line should be split in two to be inserted the piece of the column block.
      A complete width of text should be inserted in the middle of this line.
      */
      ASSERT(!CheckForTabPos(pFile, pFile->nRow + i, pFile->nCol));
      nBlockSize += *pnBlockWidth;
    }

    nBlockSize += DEFAULT_EOL_SIZE;
  }

  /*
  Allocate a block to fit the column block lines
  */
  pNewBlock = AllocTBlock();
  if (pNewBlock == NULL)
    return NULL;

  pNewBlock->pBlock = AllocateTBlock(nBlockSize);
  if (pNewBlock->pBlock == NULL)
  {
_dispose_pBlock:
    FreeTBlock(pNewBlock);
    return NULL;
  }

  pNewBlock->blockattr = COLUMN_BLOCK;
  pNewBlock->nEOLType = DEFAULT_EOL_TYPE;
  pNewBlock->nNumberOfLines = pBlock->nNumberOfLines;
  TArrayInit(pNewBlock->pIndex, pBlock->nNumberOfLines, 1);
  if (pNewBlock->pIndex == NULL)
  {
    DisposeBlock(pNewBlock->pBlock);
    goto _dispose_pBlock;
  }
  pNewBlockLine = pNewBlock->pIndex;
  p = pNewBlock->pBlock;
  memset(pNewBlock->pBlock, ' ', nBlockSize);

  /*
  Prepare each piece of column block to fit
  to a particular line.
  */
  for (i = 0; i < pBlock->nNumberOfLines; ++i)
  {
    ASSERT(pFile->nRow + i < pFile->nNumberOfLines);

    pBlockLine = GetBlockLine(pBlock, i);
    pLine = GetLine(pFile, pFile->nRow + i);

    nBlockLineLen = pBlockLine->nLen;

    nLastCol = GetTabPos(pFile, pFile->nRow + i, pLine->nLen);

    pNewBlockLine->pLine = p;
    pNewBlockLine->pFileBlock = pNewBlock->pBlock;
    pNewBlockLine->attr = 0;

    if (nLastCol < pFile->nCol)
    {
      /*
      This line should be filled up with spaces to make up to the
      column block left edge position.
      The region to be filled is from column nLastCol up to pFile->nCol.
      */
      if (nBlockLineLen > 0)
        PutBlanks(&p, nLastCol, pFile->nCol);

      strcpy(p, pBlockLine->pLine);  /* Add a line from column block */
      p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
    }
    else
    {
      /*
      The line should be split in two to be inserted the piece of the column block.
      A complete width of text should be inserted in the middle of this line.
      */
      ASSERT(!CheckForTabPos(pFile, pFile->nRow + i, pFile->nCol));

      strcpy(p, pBlockLine->pLine);  /* Add a line from column block */
      p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
      *p = ' ';  /* Remove trailing '\0' */

      /*
      Make up to full width if necessary.
      */
      p += *pnBlockWidth - pBlockLine->nLen;
      *p = '\0';
    }

    pNewBlockLine->nLen = p - pNewBlockLine->pLine;  /* Calc the line length */
    p += DEFAULT_EOL_SIZE;

    ++pNewBlockLine;
    ASSERT(p != NULL);
  }

  IncRef(pNewBlock->pBlock, pBlock->nNumberOfLines);  /* Update the file block reference counter */
  TArraySetCount(pNewBlock->pIndex, pBlock->nNumberOfLines);

  return pNewBlock;
}

/* ************************************************************************
   Function: InsertColumnBlockPrim
   Description:
     Inserts a column block at the current cursor position.
     pBlockPattern should be a result of FitColumnBlock().
*/
BOOLEAN InsertColumnBlockPrim(TFile *pFile, const TBlock *pBlockPattern)
{
  int nBlockSize;
  TBlock *pNewBlock;
  int i;
  TLine *pBlockLine;
  TLine *pLine;
  TLine *pNewBlockLine;
  int nEOLSize;
  char *p;
  char *d;
  int nLastCol;
  int nColPos;
  int nCol;
  TLine *pOldLine;  /* where begins the area of the old lines */
  int nOldLine;
  int nNumberOfLines;

  ASSERT((pBlockPattern->blockattr & COLUMN_BLOCK) != 0);

  /* Ensure that there are lines to fit all the column block lines */
  ASSERT(pFile->nRow + pBlockPattern->nNumberOfLines <= pFile->nNumberOfLines);

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return FALSE;

  ASSERT(pFile->pCurPos != NULL);
  ASSERT(INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos) >= 0);

  /*
  Walk along the lines where each piece of line of the column block
  will be inserted and calc the necessary size of the block, that
  should be allocated to fit the transformed column block.
  */
  nBlockSize = 0;
  nEOLSize = pFile->nEOLType == CRLFtype ? 2 : 1;
  for (i = 0; i < pBlockPattern->nNumberOfLines; ++i)
  {
    ASSERT(pFile->nRow + i < pFile->nNumberOfLines);

    pBlockLine = GetBlockLine(pBlockPattern, i);
    pLine = GetLine(pFile, pFile->nRow + i);
    ASSERT(pBlockLine->nLen == (int)strlen(pBlockLine->pLine));
    ASSERT(pLine->nLen == (int)strlen(pLine->pLine));

    nBlockSize += pBlockLine->nLen + pLine->nLen + nEOLSize;
  }

  /*
  Allocated a block to combine the column block pieces
  along with the correspondent lines of the file.
  */
  pNewBlock = AllocTBlock();
  if (pNewBlock == NULL)
    return FALSE;

  pNewBlock->pBlock = AllocateTBlock(nBlockSize);
  if (pNewBlock->pBlock == NULL)
  {
_dispose_pBlock:
    FreeTBlock(pNewBlock);
    return FALSE;
  }

  TArrayInit(pNewBlock->pIndex, pBlockPattern->nNumberOfLines, 1);
  if (pNewBlock->pIndex == NULL)
  {
    DisposeBlock(pNewBlock->pBlock);
    goto _dispose_pBlock;
  }
  pNewBlockLine = pNewBlock->pIndex;
  p = pNewBlock->pBlock;
  memset(p, 0, nBlockSize);  /* to easy maintain EOL markers */

  /*
  Combine each piece of column block with the text of a particular line.
  */
  for (i = 0; i < pBlockPattern->nNumberOfLines; ++i)
  {
    ASSERT(pFile->nRow + i < pFile->nNumberOfLines);

    pBlockLine = GetBlockLine(pBlockPattern, i);
    pLine = GetLine(pFile, pFile->nRow + i);

    nLastCol = GetTabPos(pFile, pFile->nRow + i, pLine->nLen);

    pNewBlockLine->pLine = p;
    pNewBlockLine->pFileBlock = pNewBlock->pBlock;
    pNewBlockLine->attr = 0;

    if (nLastCol < pFile->nCol)
    {
      /*
      Whole the line remains left of the column block.
      */
      strcpy(p, pLine->pLine);
      strcat(p, pBlockLine->pLine);
    }
    else
    {
      /*
      The column block should be inserted in the middle
      of a line.
      */
      d = pLine->pLine;
      nCol = 0;
      while (*d)  /* Walk through the line to take on count the '\t's */
      {
        if (nCol >= pFile->nCol)  /* We get to the desired column */
        {
          /* The section should have be detabed! */
          ASSERT(nCol == pFile->nCol);
          break;
        }

        if (*d == '\t')  /* Tab char */
          nCol = CalcTab(nCol);
        else
          ++nCol;
        ++d;
      }
      nColPos = d - pLine->pLine;
      strncpy(p, pLine->pLine, nColPos);
      strcat(p, pBlockLine->pLine);
      strcat(p, d);
    }
    p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
    pNewBlockLine->nLen = p - pNewBlockLine->pLine;  /* Calc the line length */
    p += nEOLSize;

    ++pNewBlockLine;
    ASSERT(p != NULL);
    ASSERT(p - pNewBlock->pBlock <= nBlockSize);
  }

  IncRef(pNewBlock->pBlock, pBlockPattern->nNumberOfLines);  /* Update the file block reference counter */
  TArraySetCount(pNewBlock->pIndex, pBlockPattern->nNumberOfLines);
  pNewBlock->nNumberOfLines = pBlockPattern->nNumberOfLines;
  pNewBlock->nEOLType = pFile->nEOLType;

  /*
  Insert all the new lines in the index array of the file.
  */
  nNumberOfLines = pNewBlock->nNumberOfLines;
  TArrayInsertGroup(pFile->pIndex, pFile->nRow,
    pNewBlock->pIndex, nNumberOfLines);
  if (!TArrayStatus(pFile->pIndex))  /* Failed to insert the lines */
  {
    TArrayClearStatus(pFile->pIndex);
    DisposeABlock(&pNewBlock);
    return FALSE;
  }
  AddBlockLink(&pFile->blist, pNewBlock->pBlock);

  /* Remove the original version of the lines that were split to insert column block */
  nOldLine = pFile->nRow + nNumberOfLines;  /* Old line position */
  for (i = 0; i < nNumberOfLines; ++i)
  {
    pOldLine = GetLine(pFile, nOldLine + i);
    DecRef(pOldLine->pFileBlock, 1);  /* As this line is to be removed */
  }
  TArrayDeleteGroup(pFile->pIndex, nOldLine, nNumberOfLines);

  /*
  As pNewBlock->pIndex was copied and all the pNewBlock parameters
  are moved into pFile dispose pNewBlock.
  */
  TArrayDispose(pNewBlock->pIndex);
  FreeTBlock(pNewBlock);

  /*
  Reestablish valid pCurPos (it might point to the block that's released)
  Use nCol/nRow which are still valid to repoint pCurPos
  */
  GotoColRow(pFile, pFile->nCol, pFile->nRow);

  if (nNumberOfLines == 1)
    pFile->bUpdateLine = TRUE;
  else
    pFile->bUpdatePage = TRUE;

  InvalidateEOLStatus(pFile, pFile->nRow);

  pFile->bUpdateStatus = TRUE;

  /* 9:46pm (31-mar-2005) and some last moment sanity checks */
  ASSERT(pFile->pCurPos != NULL);
  ASSERT(INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos) >= 0);

  return TRUE;
}

/* ************************************************************************
   Function: InsertColumnBlock
   Description:
     Inserts a column block.
     Prepares a specific column block pattern depending on inserting
     position by calling FixColumnBlock().
     Adds undo logging of the operaion.
*/
static BOOLEAN InsertColumnBlock(TFile *pFile, const TBlock *pBlock)
{
  TBlock *pBlockPattern;
  int nBlockWidth;
  BOOLEAN bResult;
  int nUndoEntry;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PBLOCK(pBlock));

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return FALSE;

  bResult = TRUE;

  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (!AddUndoRecord(pFile, acINSERT, FALSE))
  {
_failed1:
    bResult = FALSE;
    goto _exit_point;
  }

  pBlockPattern = FitColumnBlock(pFile, pBlock, &nBlockWidth);
  if (pBlockPattern == NULL)
    goto _failed1;

  RecordUndoData(pFile, pBlockPattern,
    pFile->nRow, pFile->nCol, pFile->nRow + pBlock->nNumberOfLines - 1, pFile->nCol + nBlockWidth - 1);

  bResult = InsertColumnBlockPrim(pFile, pBlockPattern);

  if (bResult)
  {
    pFile->bChanged = TRUE;
    pFile->bRecoveryStored = FALSE;
  }

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);

  return bResult;
}

/* ************************************************************************
   Function: UpdateMarkersDelete
   Description:
     Called whenever a block of characters is deleted from a file.
     Updates current cursor positions markers.
     TODO: Updates all the row related markers in the file.

   On entry:
     pFile -- update here
     nRow -- the row where the characters block was deleted
     nCPosIndex -- index of the characters position where the block was
       deleted
     nNumberOfLines -- how much lines comprises the block
     nLastLineLen -- the length of the last line of the block
*/
static void UpdateMarkersDelete(TFile *pFile, int nRow, int nCPosIndex,
  int nNumberOfLines, int nLastLineLen)
{
  /*
  Check to see if EOL status needs invalidation
  */
  if (GetEOLStatus(pFile, nRow) != pFile->lnattr)
    InvalidateEOLStatus(pFile, nRow);

  /*
  Update the cursor position.
  */
  if (nRow >= pFile->nNumberOfLines)
  {
    GotoColRow(pFile, 0, pFile->nNumberOfLines);  /* As there're no lines at nRow */
    if (pFile->nStartLine >= pFile->nRow)
    {
      /*
      Removed block at the very end of the file.
      So unmark block.
      */
      pFile->nStartLine = -1;
      pFile->nStartPos = -1;
      pFile->nEndLine = -1;
      pFile->nEndPos = -2;
      pFile->bBlock = FALSE;
      pFile->bUpdatePage = TRUE;  /* Display the new position of <*** End Of File ***> */
      return;
    }
  }
  else
    GotoPosRow(pFile, nCPosIndex, nRow);

  /*
  Update bookmarks.
  */
  if (nNumberOfLines > 0)
    BMListUpdate(pFile, nRow, -nNumberOfLines);

  UpdateFunctionNamesBookmarks(pFile, nRow, -nNumberOfLines);

  /*
  Update the block markers.
  HINT: For character blocks nStartPos/nEndPos are indices of a particular
  character positions, so they could be compared against nCPosIndex.
  */
  if (pFile->blockattr & COLUMN_BLOCK)
  {
    if (pFile->nStartLine > pFile->nNumberOfLines)
      pFile->nStartLine = pFile->nNumberOfLines - 1;
    if (pFile->nEndLine > pFile->nNumberOfLines)
      pFile->nEndLine = pFile->nNumberOfLines - 1;
    if (pFile->nStartLine < 0)
      pFile->bBlock = FALSE;
    return;
  }

  if (nRow < pFile->nStartLine)
  {
    if (pFile->nStartLine != -1)  /* if defined only */
      pFile->nStartLine -= nNumberOfLines - 1;
  }
  else
    if (nRow == pFile->nStartLine)
    {
      ASSERT(pFile->nStartPos >= 0);  /* nStartLine/nStartPos should be a valid pair */

      if (nCPosIndex < pFile->nStartPos)
        pFile->nStartPos -= nLastLineLen;
    }

  if (nRow < pFile->nEndLine)
  {
    if (pFile->nEndLine != -1)  /* if defined only */
    {
      if (nRow + nNumberOfLines - 1 == pFile->nEndLine)
        pFile->nEndPos -= nLastLineLen;  /* End-of-deleted-area intersection */
      pFile->nEndLine -= nNumberOfLines - 1;
    }
  }
  else
    if (nRow == pFile->nEndLine)
    {
      ASSERT(pFile->nEndPos >= -1);  /* nEndLine/nEndPos should be a valid pair */

      if (nCPosIndex <= pFile->nEndPos)
        pFile->nEndPos -= nLastLineLen;
    }

  /*
  Check for block anihilation
  */
  if (pFile->bBlock)
  {
    if (pFile->nStartLine < pFile->nEndLine)
      pFile->bBlock = FALSE;
    if (pFile->nStartLine == pFile->nEndLine)
      if (pFile->nEndPos < pFile->nStartPos)
        pFile->bBlock = FALSE;
  }
}

/* ************************************************************************
   Function: DeleteCharacterBlockPrim
   Description:
     Deletes a specific area from pFile.
*/
BOOLEAN DeleteCharacterBlockPrim(TFile *pFile,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos)
{
  int nBlockSize;
  TLine *pLine1;
  TLine *pLine2;
  int nEOLSize;
  TBlock *pNewBlock;
  TLine *pNewBlockLine;
  char *p;
  int nNumberOfLines;
  int nOldLine;
  TLine *pOldLine;
  int i;
  int nLastLineLen;
  int nLen2;
  int nDeleteLines;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nStartLine >= 0);
  ASSERT(nEndLine >= 0);
  ASSERT(nEndLine >= nStartLine);
  ASSERT(nEndLine <= pFile->nNumberOfLines);
  ASSERT(pFile->nNumberOfLines >= 1);

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return FALSE;

  pFile->lnattr = GetEOLStatus(pFile, nStartLine);

  /*
  Make a block that concatenates 2 portions of text:
  portion 1: From nStartLine, the characters from start of the line
  up to nStartPos;
  -and-
  portion 2: From nEndLine, the characters from nEndPos up to the end
  of the line;
  */
  pLine1 = GetLine(pFile, nStartLine);
  pLine2 = GetLine(pFile, nEndLine);
  ASSERT(nStartPos <= pLine1->nLen);
  nLen2 = 0;
  nNumberOfLines = nEndLine - nStartLine;
  if (pLine2 != NULL)  /* == NULL when at <*** end of file ***> line */
  {
    ASSERT(nEndPos <= pLine2->nLen);
    nLen2 = pLine2->nLen;
  }
  else
    pFile->bUpdatePage = TRUE;  /* As a line is removed */
  nEOLSize = (pFile->nEOLType == CRLFtype) ? 2 : 1;
  nBlockSize = nStartPos + nLen2 - (nEndPos + 1);
  if (nBlockSize == 0)  /* No lines to join after deletion */
  {
    /*
    Check for a special case when afer deleting remains nothing
    in the file.
    */
    if (nEndLine == pFile->nNumberOfLines && nStartLine == 0)
    {
      DisposeBlockList(&pFile->blist);
      if (pFile->pIndex != NULL)
        TArrayDispose(pFile->pIndex);
      pFile->bUpdatePage = TRUE;
      nLastLineLen = 0;
      nNumberOfLines = pFile->nNumberOfLines;
      pFile->nNumberOfLines = 0;
      goto _update_markers;
    }
    /*
    Special case, deleting and no contents to join
    */
    if (nNumberOfLines > 0)
      goto _remove_lines;  /* Just remove */
  }
  nBlockSize += nEOLSize;
  nLastLineLen = nEndPos + 1;
  if (nStartLine == nEndLine)
    nLastLineLen -= nStartPos;

  /*
  Allocated a block to combine the column block pieces
  along with the correspondent lines of the file.
  */
  pNewBlock = AllocTBlock();
  if (pNewBlock == NULL)
    return FALSE;

  pNewBlock->pBlock = AllocateTBlock(nBlockSize);
  if (pNewBlock->pBlock == NULL)
  {
_dispose_pBlock:
    FreeTBlock(pNewBlock);
    return FALSE;
  }

  TArrayInit(pNewBlock->pIndex, 1, 1);
  if (pNewBlock->pIndex == NULL)
  {
    DisposeBlock(pNewBlock->pBlock);
    goto _dispose_pBlock;
  }
  pNewBlockLine = pNewBlock->pIndex;
  p = pNewBlock->pBlock;
  memset(p, 0, nBlockSize);  /* to easy maintain EOL markers */

  pNewBlockLine->pLine = p;
  pNewBlockLine->pFileBlock = pNewBlock->pBlock;
  pNewBlockLine->attr = 0;

  strncpy(p, pLine1->pLine, nStartPos);
  if (pLine2 != NULL)  /* == NULL when at <*** end of file ***> line */
    strcat(p, pLine2->pLine + nEndPos + 1);

  p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
  pNewBlockLine->nLen = p - pNewBlockLine->pLine;  /* Calc the line length */
  p += nEOLSize;

  ++pNewBlockLine;
  ASSERT(p != NULL);
  ASSERT(p - pNewBlock->pBlock == nBlockSize);
  ASSERT(pNewBlockLine - pNewBlock->pIndex == 1);

  IncRef(pNewBlock->pBlock, 1);  /* Update the file block reference counter */
  TArraySetCount(pNewBlock->pIndex, 1);
  pNewBlock->nNumberOfLines = 1;
  pNewBlock->nEOLType = pFile->nEOLType;

  /*
  Insert the new line in the index array of the file.
  */
  if (pLine2 == NULL)
    --nNumberOfLines;  /* We actually have to replace the last line! */
  if (nNumberOfLines == 0)
  {
    /* Special case when region is contained in a single line */
    pOldLine = GetLine(pFile, nStartLine);
    if (pLine2 != NULL)
      ASSERT(nStartLine == nEndLine);
    DecRef(pOldLine->pFileBlock, 1);
    pFile->pIndex[nStartLine] = pNewBlock->pIndex[0];
    AddBlockLink(&pFile->blist, pNewBlock->pBlock);
    goto _dispose_newblock;
  }
  /* Multi-line block is deleted */
  TArrayInsertGroup(pFile->pIndex, nStartLine, pNewBlock->pIndex, 1);
  if (!TArrayStatus(pFile->pIndex))  /* Failed to insert the lines */
  {
    TArrayClearStatus(pFile->pIndex);
    DisposeABlock(&pNewBlock);
    return FALSE;
  }
  AddBlockLink(&pFile->blist, pNewBlock->pBlock);

  /*
  Remove all lines in the range nStartLine..nEndLine
  */
_remove_lines:
  pFile->nNumberOfLines -= nNumberOfLines;
  if (nBlockSize == 0)
  {
    nOldLine = nStartLine;  /* Old line position after inserting 1 new line */
    nDeleteLines = nNumberOfLines;
  }
  else
  {
    nOldLine = nStartLine + 1;  /* Old line position after inserting 1 new line */
    if (pLine2 == NULL)  /* We can not join with <*** end of file ***> line */
    {
      ++pFile->nNumberOfLines;  /* Make up for nothing to join with! */
      --nNumberOfLines;  /* As we insert some text and do not join lines */
    }
    nDeleteLines = nNumberOfLines + 1;
  }
  for (i = 0; i < nDeleteLines; ++i)
  {
    pOldLine = GetLine(pFile, nOldLine + i);
    DecRef(pOldLine->pFileBlock, 1);  /* As this line is to be removed */
  }
  TArrayDeleteGroup(pFile->pIndex, nOldLine, nDeleteLines);

  /*
  As pNewBlock->pIndex was copied and all the pNewBlock parameters
  are moved into pFile dispose pNewBlock.
  */
_dispose_newblock:
  if (nBlockSize != 0)
  {
    TArrayDispose(pNewBlock->pIndex);
    FreeTBlock(pNewBlock);
  }

  /*
  Update the cursor position to point at the end of the block.
  Update any row related markers.
  */
_update_markers:
  UpdateMarkersDelete(pFile, nStartLine, nStartPos, nNumberOfLines, nLastLineLen);

  if (nNumberOfLines == 0)
    pFile->bUpdateLine = TRUE;
  else
    pFile->bUpdatePage = TRUE;

  pFile->bUpdateStatus = TRUE;

  return TRUE;
}

/* ************************************************************************
   Function: DeleteCharacterBlock
   Description:
     Deletes a character block.
     This is a wrap for DeleteCharacterBlockPrim().
     Adds undo logging of the operaion.
*/
BOOLEAN DeleteCharacterBlock(TFile *pFile,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos)
{
  BOOLEAN bResult;
  int nUndoEntry;
  TBlock *pBlockCopy;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nStartLine >= 0);
  ASSERT(nEndLine >= 0);
  ASSERT(nEndLine >= nStartLine);
  ASSERT(nEndLine <= pFile->nNumberOfLines);
  ASSERT(pFile->nNumberOfLines >= 1);

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return FALSE;

  bResult = TRUE;

  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (!AddUndoRecord(pFile, acDELETE, FALSE))
  {
_failed1:
    bResult = FALSE;
    goto _exit_point;
  }

  pBlockCopy = MakeACopyOfBlock(pFile, nStartLine, nEndLine, nStartPos, nEndPos, 0);
  if (pBlockCopy == NULL)
    goto _failed1;

  RecordUndoData(pFile, pBlockCopy, nStartLine, nStartPos, nEndLine, nEndPos);
  bResult = DeleteCharacterBlockPrim(pFile, nStartLine, nStartPos, nEndLine, nEndPos);

  if (bResult)
  {
    pFile->bChanged = TRUE;
    pFile->bRecoveryStored = FALSE;
  }

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);
  return bResult;
}

/* ************************************************************************
   Function: DeleteColumnBlockPrim
   Description:
     Deletes a column block area.
     The function is to be used in 2 different contexts
     1. The function is called from DeleteColumnBlock(), in this case
     bGeneratePattern should be TRUE. DeleteColumnBlockPrim() will
     put in pBlockPattern the region of the file that is removed.
     2. The function is called from Undo(), in this case
     bGeneratePattern should be FALSE. DeleteColumnBlockPrim() will
     use pBlokcPattern to precisize the exact region that is to be
     deleted.
*/
BOOLEAN DeleteColumnBlockPrim(TFile *pFile,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos,
  BOOLEAN bGeneratePattern, TBlock **pBlockPattern)
{
  int nBlockSize;
  int i;
  int nEOLSize;
  int nBlockWidth;
  int nToRemain;
  int nLineLen;
  int nLastCol;
  char *p;
  TBlock *pNewBlock;
  int nNumberOfLines;
  TLine *pNewBlockLine;
  TLine *pLine;
  char *pBlankSection;
  int nCol;
  char *pPart1;
  char *pPart2;
  char *pEndPart1;
  char *d;
  int nOldLine;
  TLine *pOldLine;
  int nPatternSize;
  char *p2;
  TLine *pPatternLine;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nStartLine >= 0);
  ASSERT(nEndLine >= nStartLine);
  ASSERT(nEndLine < pFile->nNumberOfLines);
  ASSERT(nStartPos >= 0);
  ASSERT(nEndPos >= 0);
  ASSERT(nEndPos >= nStartPos);
  ASSERT(pBlockPattern != NULL);

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return FALSE;

  /*
  Calc the size of the block where to compose the lines
  after deleting the column block area
  */
  nBlockSize = 0;
  if (bGeneratePattern)
    nPatternSize = 0;
  else
    nPatternSize = -1;
  nEOLSize = (pFile->nEOLType == CRLFtype ? 2 : 1);
  for (i = nStartLine; i <= nEndLine; ++i)
  {
    nLineLen = GetLine(pFile, i)->nLen;

    if (!bGeneratePattern)
    {
      /*
      We use a pattern, so we know precisely
      how much of the line will be deleted
      */
      pPatternLine = GetBlockLine((*pBlockPattern), i - nStartLine);
      nToRemain = nLineLen - pPatternLine->nLen;
      ASSERT(nToRemain >= 0);
      nBlockSize += nToRemain + nEOLSize;
      continue;
    }

    nLastCol = GetTabPos(pFile, i, nLineLen) - 1;

    if (nLastCol < nStartPos)
    {
      /*
      No portion of column block at this line
      Check for blank spaces at the end of the line
      */
      p = GetLineText(pFile, i);
      pBlankSection = strchr(p, '\0');  /* Check starting from end of the line */
      while (isspace(*(pBlankSection - 1)) && pBlankSection > p)
        --pBlankSection;
      p = strchr(p, '\0');  /* Get end of the line for calculations */
      if (bRemoveTrailingBlanks)
        nToRemain = nLineLen - (p - pBlankSection);
      else
        nToRemain = nLineLen;
    }
    else
    {
      p = GetLineText(pFile, i);
      pBlankSection = NULL;  /* Still no section */
      nCol = 0;
      while (*p)
      {
        if (nCol >= nStartPos)  /* We get to the desired column */
        {
          /* The section should have been detabed! */
          ASSERT(nCol == nStartPos);
          break;
        }

        if (isspace(*p))
        {
          if (pBlankSection == NULL)
            pBlankSection = p;  /* Mark the start of a blank section */
        }
        else
          pBlankSection = NULL;  /* We come across a non-blank character */

        if (*p == '\t')  /* Tab char */
          nCol = CalcTab(nCol);
        else
          nCol++;
        p++;
      }

      if (nLastCol > nEndPos)
        nBlockWidth = nEndPos - nStartPos + 1;
      else
      {
        /* Line ends in the range nStartPos..nEndPos */
        nBlockWidth = nLastCol - nStartPos + 1;

        if (pBlankSection != NULL && bRemoveTrailingBlanks)
          nBlockWidth += p - pBlankSection;
      }

      nToRemain = nLineLen - nBlockWidth;
    }


    nBlockSize += nToRemain + nEOLSize;
    if (bGeneratePattern)
      nPatternSize += nLineLen - nToRemain + nEOLSize;
  }

  /*
  Allocate a block to compose new lines excluding the
  column block portion
  */
  pNewBlock = AllocTBlock();
  if (pNewBlock == NULL)
    return FALSE;

  pNewBlock->pBlock = AllocateTBlock(nBlockSize);
  if (pNewBlock->pBlock == NULL)
  {
_dispose_pBlock:
    FreeTBlock(pNewBlock);
    return FALSE;
  }

  if (bGeneratePattern)
  {
    ASSERT(nPatternSize > 0);
    *pBlockPattern = AllocTBlock();
    if (*pBlockPattern == NULL)
      goto _dispose_pBlock;

    (*pBlockPattern)->pBlock = AllocateTBlock(nPatternSize);
    if ((*pBlockPattern)->pBlock == NULL)
    {
      DisposeBlock(pNewBlock->pBlock);
_dispose_pPattern:
      FreeTBlock(*pBlockPattern);
      goto _dispose_pBlock;
    }
  }

  nNumberOfLines = nEndLine - nStartLine + 1;
  pNewBlock->blockattr = COLUMN_BLOCK;
  pNewBlock->nEOLType = pFile->nEOLType;
  pNewBlock->nNumberOfLines = nNumberOfLines;
  TArrayInit(pNewBlock->pIndex, nNumberOfLines, 1);
  if (pNewBlock->pIndex == NULL)  /* no enough memory */
  {
    DisposeBlock(pNewBlock->pBlock);
    if (bGeneratePattern)
      goto _dispose_pPattern;
    else
      goto _dispose_pBlock;
  }
  pNewBlockLine = pNewBlock->pIndex;
  p = pNewBlock->pBlock;
  memset(pNewBlock->pBlock, '\0', nBlockSize);

  pPatternLine = NULL;
  p2 = NULL;
  if (bGeneratePattern)
  {
    (*pBlockPattern)->blockattr = COLUMN_BLOCK;
    (*pBlockPattern)->nEOLType = pFile->nEOLType;
    (*pBlockPattern)->nNumberOfLines = nNumberOfLines;
    TArrayInit((*pBlockPattern)->pIndex, nNumberOfLines, 1);
    if ((*pBlockPattern)->pIndex == NULL)  /* no enough memory */
    {
      DisposeBlock(pNewBlock->pBlock);
      DisposeBlock((*pBlockPattern)->pBlock);
      goto _dispose_pPattern;
    }
    pPatternLine = (*pBlockPattern)->pIndex;
    p2 = (*pBlockPattern)->pBlock;
    memset((*pBlockPattern)->pBlock, '\0', nPatternSize);
  }

  /*
  Walk along all the lines containing the column block
  and compose in pNewBlock.
  */
  for (i = nStartLine; i <= nEndLine; ++i)
  {
    pLine = GetLine(pFile, i);
    nLineLen = pLine->nLen;
    nLastCol = GetTabPos(pFile, i, nLineLen) - 1;

    pNewBlockLine->pLine = p;
    pNewBlockLine->pFileBlock = pNewBlock->pBlock;
    pNewBlockLine->attr = 0;

    if (bGeneratePattern)
    {
      ASSERT(pPatternLine != NULL);
      ASSERT(p2 != NULL);
      pPatternLine->pLine = p2;
      pPatternLine->pFileBlock = (*pBlockPattern)->pBlock;
      pPatternLine->attr = 0;
    }

    pPart1 = pLine->pLine;
    pEndPart1 = NULL;
    pPart2 = NULL;

    d = pLine->pLine;
    pBlankSection = NULL;  /* Still no section */
    nCol = 0;
    while (*d)
    {
      if (nCol == nStartPos)  /* We get to the desired column */
        pEndPart1 = d;

      if (nCol == nEndPos)
        pPart2 = d + 1;

      if (nCol < nStartPos)
      {
        /* Blank sections should be marked only prior getting to nStartPos */
        if (isspace(*d))
        {
          if (pBlankSection == NULL)
            pBlankSection = d;  /* Mark the start of a blank section */
        }
        else
          pBlankSection = NULL;  /* We come across a non-blank character */
      }

      if (*d == '\t')  /* Tab char */
        nCol = CalcTab(nCol);
      else
        nCol++;
      d++;
    }

    /*
    We walked through whole the line to determine
    pPart1, pEndPart1 and pPart2
    Part 1 is from position 0 up to nStartPos.
    Part 2 is from nEndPos up to the end of the line.
    These variables show how the column block is suited in the
    current line text content.
    We could have 3 cases:
    1) All the line is left from nStartPos (pEndPart1 undetermined)
    2) The column block separates the line in two (pPart2 determined)
    3) The line ends somwhere in between nStartPos..nEndPos (pPart2 undetermined)
    */
    if (pEndPart1 == NULL)  /* case 1. */
      pEndPart1 = d;

    if (pPart2 != NULL)
      if (*pPart2 == '\0')  /* this should be considered case 3 as well */
        pPart2 = NULL;

    if (bRemoveTrailingBlanks && pPart2 == NULL)
    {
      /* Only if removing column block will result in spaces
      at the end of the line */
      if (pBlankSection != NULL)
        pEndPart1 = pBlankSection;
    }

    if (!bGeneratePattern && pPart2 == NULL)
    {
      /*
      Here we have to use the supplied column block pattern as a hint
      to decide how much to remove from this particular line.
      So pEndPart1 as determined up to now could be changed!
      */
      pPatternLine = GetBlockLine((*pBlockPattern), i - nStartLine);
      d = strchr(pLine->pLine, '\0');  /* Find end of the line position */
      ASSERT(pPatternLine->nLen <= (d - pLine->pLine));
      pEndPart1 = d - pPatternLine->nLen;
    }

    /*
    Combine pPart1 and pPart2 (if pPart2 is present)
    */
    strncpy(p, pPart1, pEndPart1 - pPart1);
    if (pPart2 != NULL)
      strcat(p, pPart2);
    p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */

    pNewBlockLine->nLen = p - pNewBlockLine->pLine;  /* Calc the line length */
    p += nEOLSize;

    ++pNewBlockLine;
    ASSERT(p != NULL);

    if (bGeneratePattern)
    {
      if (pPart2 != NULL)  /* A portion in the middle is deleted */
        strncpy(p2, pEndPart1, pPart2 - pEndPart1);
      else  /* All up to the end of the line is deleted */
        strcpy(p2, pEndPart1);
      p2 = strchr(p2, '\0');  /* Search for end of line already marked with '\0' */

      pPatternLine->nLen = p2 - pPatternLine->pLine;  /* Calc the line length */
      p2 += nEOLSize;
      ++pPatternLine;
      ASSERT(p2 != NULL);
    }
  }

  ASSERT(p - pNewBlock->pBlock == nBlockSize);

  IncRef(pNewBlock->pBlock, nNumberOfLines);  /* Update the file block reference counter */
  TArraySetCount(pNewBlock->pIndex, nNumberOfLines);

  if (bGeneratePattern)
  {
    ASSERT(p2 - (*pBlockPattern)->pBlock == nPatternSize);
    IncRef((*pBlockPattern)->pBlock, nNumberOfLines);  /* Update the reference counter */
    TArraySetCount((*pBlockPattern)->pIndex, nNumberOfLines);
  }

  /*
  Insert all the new lines in the index array of the file.
  */
  TArrayInsertGroup(pFile->pIndex, nStartLine, pNewBlock->pIndex, nNumberOfLines);
  if (!TArrayStatus(pFile->pIndex))  /* Failed to insert the lines */
  {
    TArrayClearStatus(pFile->pIndex);
    DisposeABlock(&pNewBlock);
    return FALSE;
  }
  AddBlockLink(&pFile->blist, pNewBlock->pBlock);

  /* Remove the original version of the lines that were split to insert column block */
  nOldLine = nStartLine + nNumberOfLines;  /* Old line position */
  for (i = 0; i < nNumberOfLines; ++i)
  {
    pOldLine = GetLine(pFile, nOldLine + i);
    DecRef(pOldLine->pFileBlock, 1);  /* As this line is to be removed */
  }
  TArrayDeleteGroup(pFile->pIndex, nOldLine, nNumberOfLines);

  /*
  As pNewBlock->pIndex was copied and all the pNewBlock parameters
  are moved into pFile dispose pNewBlock.
  */
  TArrayDispose(pNewBlock->pIndex);
  FreeTBlock(pNewBlock);

  if (nNumberOfLines == 1)
    pFile->bUpdateLine = TRUE;
  else
    pFile->bUpdatePage = TRUE;

  InvalidateEOLStatus(pFile, nStartLine);
  pFile->bUpdateStatus = TRUE;

  return TRUE;
}

/* ************************************************************************
   Function: DeleteColumnBlock
   Description:
     Deletes a column block.
     The area should be detabed before calling this function.
     Adds undo logging of the operaion.
*/
BOOLEAN DeleteColumnBlock(TFile *pFile,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos)
{
  TBlock *pBlockPattern;
  int nUndoEntry;
  BOOLEAN bResult;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nStartLine >= 0);
  ASSERT(nEndLine >= nStartLine);
  ASSERT(nStartPos >= 0);
  ASSERT(nEndPos >= nStartPos);

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return FALSE;

  bResult = TRUE;

  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (!AddUndoRecord(pFile, acDELETE, FALSE))
  {
    bResult = FALSE;
    goto _exit_point;
  }

  bResult = DeleteColumnBlockPrim(pFile,
    nStartLine, nStartPos, nEndLine, nEndPos, TRUE, &pBlockPattern);

  if (!bResult)
    goto _exit_point;

  GotoColRow(pFile, nStartPos, nStartLine);  /* Undo will insert here */

  RecordUndoData(pFile, pBlockPattern, nStartLine, nStartPos, nEndLine, nEndPos);

  pFile->bChanged = TRUE;
  pFile->bRecoveryStored = FALSE;

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);

  return bResult;
}

/* ************************************************************************
   Function: MarkBlockBegin
   Description:
     Marks start character of a block. If there's valid end point
     set bBlock flag to TRUE.
*/
void MarkBlockBegin(TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));
  ASSERT(pFile->nRow <= pFile->nNumberOfLines);

  if (pFile->pCurPos == NULL)  /* At <*** End Of File ***> line */
  {
    pFile->bBlock = FALSE;
    /* Set block start coordinates to be invalid */
    pFile->nStartLine = -1;
    pFile->nStartPos = -1;
    return;
  }

  pFile->nStartLine = pFile->nRow;
  if (pFile->blockattr & COLUMN_BLOCK)
    pFile->nStartPos = pFile->nCol;
  else
    pFile->nStartPos = INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos);

  /*
  Check whether the block is valid
  */
  pFile->bBlock = FALSE;  /* assume invalid */
  pFile->bUpdatePage = TRUE;

  if (pFile->nStartLine == -1 || pFile->nEndLine == -1)
    return;

  if (pFile->nStartLine > pFile->nEndLine)
    return;

  if (pFile->blockattr & COLUMN_BLOCK)
  {
    if (pFile->nStartPos <= pFile->nEndPos)
      pFile->bBlock = TRUE;
    return;
  }

  if (pFile->nStartLine == pFile->nEndLine)
    if (pFile->nStartPos > pFile->nEndPos)
      return;

  pFile->bBlock = TRUE;
}

/* ************************************************************************
   Function: MarkBlockEnd
   Description:
     Marks the end character of a block. If there's valid end point
     set bBlock flag to TRUE.
*/
void MarkBlockEnd(TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));
  ASSERT(pFile->nRow <= pFile->nNumberOfLines);

  pFile->nEndLine = pFile->nRow;
  if (pFile->blockattr & COLUMN_BLOCK)
  {
    pFile->nEndPos = pFile->nCol;
    if (pFile->nRow == pFile->nNumberOfLines)
      --pFile->nEndLine;
  }
  else
    if (pFile->pCurPos == NULL)  /* At <*** End Of File ***> line */
      pFile->nEndPos = -1;
    else
      pFile->nEndPos = INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos) - 1;

  /*
  Check whether the block is valid
  */
  pFile->bBlock = FALSE;  /* assume invalid */
  pFile->bUpdatePage = TRUE;

  if (pFile->nStartLine == -1 || pFile->nEndLine == -1)
    return;

  if (pFile->nStartLine > pFile->nEndLine)
    return;

  if (pFile->blockattr & COLUMN_BLOCK)
  {
    if (pFile->nStartPos <= pFile->nEndPos)
      pFile->bBlock = TRUE;
    return;
  }

  if (pFile->nStartLine == pFile->nEndLine)
    if (pFile->nStartPos > pFile->nEndPos)
      return;

  pFile->bBlock = TRUE;
}

/* ************************************************************************
   Function: ValidBlockMarkers
   Description:
     Verifies whether block-start/block-end markers form a valid
     block
*/
BOOLEAN ValidBlockMarkers(TFile *pFile)
{
  if (pFile->nStartLine == -1 || pFile->nEndLine == -1)
    return FALSE;

  if (pFile->nStartLine > pFile->nEndLine)
    return FALSE;

  if (pFile->blockattr & COLUMN_BLOCK)
  {
    if (pFile->nStartPos <= pFile->nEndPos)
      return TRUE;
    return FALSE;
  }

  if (pFile->nStartLine == pFile->nEndLine)
    if (pFile->nStartPos > pFile->nEndPos)
      return FALSE;

  return TRUE;
}

/* ************************************************************************
   Function: ToggleBlockHide
   Description:
     Hides/Unhides a block
*/
void ToggleBlockHide(TFile *pFile)
{
  if (pFile->bBlock)
  {
    pFile->bBlock = FALSE;  /* Hide the block */
    pFile->bUpdatePage = TRUE;
    return;
  }

  /*
  Unhide the block: Check whether the block is valid
  */
  if (ValidBlockMarkers(pFile))
  {
    pFile->bBlock = TRUE;
    pFile->bUpdatePage = TRUE;
  }
}

#define sign(n)  (n == 0 ? 0 : (n > 0 ? 1 : -1))

#define ___min(a, b)  ((a) < (b) ? (a) : (b))
#define ___max(a, b)  ((a) > (b) ? (a) : (b))

/* ************************************************************************
   Function: CompareColRowPositions
   Description:
     Compares whether two col,row positions are <, > or =
     and returns -1, 1 and 0 respectively.
*/
int CompareColRowPositions(int nCol1, int nRow1, int nCol2, int nRow2)
{
  if (nRow1 == nRow2)
  {
    return (sign(nCol1 - nCol2));
  }
  return (sign(nRow1 - nRow2));
}

/* ************************************************************************
   Function: BeginBlockExtend
   Description:
*/
void BeginBlockExtend(TFile *pFile)
{
  if (pFile->nCol != pFile->nExpandCol ||
      pFile->nRow != pFile->nExpandRow)
  {
    /* Start new mark */
    MarkBlockBegin(pFile);
    MarkBlockEnd(pFile);  /* Mark an empty block */
    pFile->nExtendAncorCol = pFile->nCol;
    pFile->nExtendAncorRow = pFile->nRow;
  }
  pFile->nExpandCol = pFile->nLastExtendCol = pFile->nCol;
  pFile->nExpandRow = pFile->nLastExtendRow = pFile->nRow;
}

/* ************************************************************************
   Function: EndBlockExtend
   Description:
   KNOWN PROBLEM:
     It is impossible when at <*** End Of File ***> line and by
     pressing Shift+navigation to mark a block (you have to mark the
     last line). Although the last line can be marked in the opposite
     direction. Well, this can be fixed if found annoying.
*/
void EndBlockExtend(TFile *pFile)
{
  int nLastDir;
  int nPresentDir;
  int nBlockX1;  /* Column block coordinates */
  int nBlockY1;
  int nBlockX2;
  int nBlockY2;
  int nStart;  /* Column block start and end lines */
  int nEnd;

  pFile->nExpandCol = pFile->nCol;
  pFile->nExpandRow = pFile->nRow;
  pFile->bPreserveSelection = TRUE;

  if ((pFile->blockattr & COLUMN_BLOCK) != 0)
  {
    nBlockX1 = ___min(pFile->nExtendAncorCol, pFile->nExpandCol);
    nBlockY1 = ___min(pFile->nExtendAncorRow, pFile->nExpandRow);
    nBlockX2 = ___max(pFile->nExtendAncorCol, pFile->nExpandCol);
    nBlockY2 = ___max(pFile->nExtendAncorRow, pFile->nExpandRow);
    nStart = pFile->nStartLine;
    nEnd = pFile->nEndLine;
    if (nStart != nBlockY1 || pFile->nStartPos != nBlockX1)
    {
      GotoColRow(pFile, nBlockX1, nBlockY1);
      MarkBlockBegin(pFile);
    }
    if (nEnd != nBlockY2 || pFile->nEndPos != nBlockX2)
    {
      GotoColRow(pFile, nBlockX2, nBlockY2);
      MarkBlockEnd(pFile);
    }
    GotoColRow(pFile, pFile->nExpandCol, pFile->nExpandRow);  /* Return to present cursor position */
    return;
  }

  /*
  Continuous character block
  */
  nLastDir =
    CompareColRowPositions(
      pFile->nLastExtendCol,
      pFile->nLastExtendRow,
      pFile->nExtendAncorCol,
      pFile->nExtendAncorRow
    );
  nPresentDir =
    CompareColRowPositions(pFile->nCol, pFile->nRow,
      pFile->nExtendAncorCol, pFile->nExtendAncorRow);

  if (nLastDir != nPresentDir)
  {
    if (nLastDir == -1)
    {
      GotoColRow(pFile, pFile->nExtendAncorCol, pFile->nExtendAncorRow);
      MarkBlockBegin(pFile);
      GotoColRow(pFile, pFile->nExpandCol, pFile->nExpandRow);
      MarkBlockEnd(pFile);
      return;
    }
    if (nLastDir == 1)
    {
      GotoColRow(pFile, pFile->nExtendAncorCol, pFile->nExtendAncorRow);
      MarkBlockEnd(pFile);
      GotoColRow(pFile, pFile->nExpandCol, pFile->nExpandRow);
      MarkBlockBegin(pFile);
      return;
    }
  }

  if (nPresentDir == -1)
    MarkBlockBegin(pFile);
  else
    MarkBlockEnd(pFile);
}

/* ************************************************************************
   Function: ReplaceTextPrim
   Description:
     This routine replaces portion of text in a file with new text.
     There should have be enough characters at pFile->pCurPos to be replaced
     by pText!

     NOTE: To be used for undo purposes this function stores in pOldText
     what is to be replaced by pText. No additional memory necessary,
     which means that pText and pOldText may point to one and the same
     memory location.
*/
void ReplaceTextPrim(TFile *pFile, const char *pText, char *pOldText)
{
  int nLen;
  int i;
  char c;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(pFile->pCurPos != NULL);
  ASSERT(pText != NULL);
  ASSERT(pText[0] != '\0');
  /* below: Ensure there's enough characters after pCurPos to put all from pText */
  ASSERT(strchr(pFile->pCurPos, '\0') - pFile->pCurPos >= (int)strlen(pText));

  nLen = strlen(pText);

  for (i = 0; i < nLen; ++i)
  {
    c = pFile->pCurPos[i];
    pFile->pCurPos[i] = pText[i];
    if (pOldText != NULL)
      pOldText[i] = c;
  }

  GotoColRow(pFile, pFile->nCol + 1, pFile->nRow);
}

/* ************************************************************************
   Function: GenerateBlock
   Description:
     Generates a block filled with a specified symbol.
     Returns NULL if no memory to complete the operation.
*/
static TBlock *GenerateBlock(char c, int nSize)
{
  TBlock *pBlock;
  TLine *pLn;
  char *p;

  pBlock = AllocTBlock();
  if (pBlock == NULL)
    return NULL;

  pBlock->pBlock = AllocateTBlock(nSize + DEFAULT_EOL_SIZE);
  if (pBlock->pBlock == NULL)
  {
_dispose_pBlock:
    FreeTBlock(pBlock);
    return NULL;
  }
  memset(pBlock->pBlock, c, nSize);
  pBlock->pBlock[nSize] = '\0';

  pBlock->nEOLType = DEFAULT_EOL_TYPE;
  pBlock->nNumberOfLines = 1;  /* character block is always +1 more line */
  pBlock->blockattr = 0;

  TArrayInit(pBlock->pIndex, pBlock->nNumberOfLines, 1);
  if (pBlock->pIndex == NULL)
  {
    DisposeBlock(pBlock->pBlock);
    goto _dispose_pBlock;
  }

  p = pBlock->pBlock;
  pLn = pBlock->pIndex;

  pLn->pLine = p;
  pLn->pFileBlock = pBlock->pBlock;  /* pLine is a reference inside this block */
  pLn->attr = 0;

  p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
  pLn->nLen = p - pLn->pLine;  /* Calc the line length */

  IncRef(pBlock->pBlock, pBlock->nNumberOfLines);  /* Update the file block reference counter */
  TArraySetCount(pBlock->pIndex, pBlock->nNumberOfLines);

  return pBlock;
}

/* ************************************************************************
   Function: DetabColumnRegion
   Description:
     Replaces all the tab characters from a square region
     with relevant amount of spaces.
*/
BOOLEAN DetabColumnRegion(TFile *pFile,
  int nStartLine, int nStartCol, int nEndLine, int nEndCol)
{
  int i;
  int d;
  int dend;
  int nCol;
  int nToEndCol;
  char *pLine;
  TBlock *pBlock;
  int nPrevCol;
  int nSpaces;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nStartLine >= 0);
  ASSERT(nEndLine <= pFile->nNumberOfLines);
  ASSERT(nStartLine <= nEndLine);
  ASSERT(nStartCol >= 0);
  ASSERT(nEndCol >= 0);

  for (i = nStartLine; i <= nEndLine; ++i)
  {
    pLine = GetLineText(pFile, i);
    nCol = 0;
    d = 0;
    nPrevCol = 0;
    while (pLine[d] && nCol < nEndCol)
    {
      if (pLine[d] == '\t')  /* Tab char */
      {
        nCol = CalcTab(nCol);
        if (nCol > nStartCol)
        {
          /*
          Tab character inside the specified region nStartPos..nEndPos
          Determine how much consequtive tabs to remove.
          */
          dend = d;
          nToEndCol = nCol;
          while (pLine[++dend] == '\t' && nToEndCol < nEndCol)
            nToEndCol = CalcTab(nToEndCol);

          if (!DeleteCharacterBlock(pFile, i, d, i, dend - 1))
            return FALSE;

          nSpaces = nToEndCol - nPrevCol;
          pBlock = GenerateBlock(' ', nSpaces);
          if (pBlock == NULL)
            return FALSE;
          GotoPosRow(pFile, d, i);
          if (!InsertCharacterBlock(pFile, pBlock, -1, -1))
          {
            DisposeABlock(&pBlock);
            return FALSE;
          }
          DisposeABlock(&pBlock);

          /*
          Reassign pLine and other local line pointers as the text was changed
          */
          pLine = GetLineText(pFile, i);
          d += nSpaces - 1;
          nCol = nToEndCol;
        }
      }
      else
        nCol++;
      d++;
      nPrevCol = nCol;
    }
  }

  if (nEndLine - nStartLine == 0)
    pFile->bUpdateLine = TRUE;
  else
    pFile->bUpdatePage = TRUE;

  return TRUE;
}

/* ************************************************************************
   Function: MakeBlock
   Description:
     Allocates a string as TBlock.
     This function does nearly the same like LoadFilePrim() to process
     the string. The string can be compaunded of 1 or more lines. End-Of-Line
     separators are detected and processed properly.
*/
TBlock *MakeBlock(const char *s, WORD blockattr)
{
  TBlock *pBlock;
  TLine *pLn;
  char *p;
  char *pSentinel;
  BOOLEAN bStop;

  ASSERT(s != NULL);

  pBlock = AllocTBlock();
  if (pBlock == NULL)
    return NULL;

  pBlock->pBlock = AllocateTBlock(strlen(s) + DEFAULT_EOL_SIZE);
  if (pBlock->pBlock == NULL)
  {
_dispose_pBlock:
    FreeTBlock(pBlock);
    return NULL;
  }
  strcpy(pBlock->pBlock, s);

  pBlock->nEOLType = DEFAULT_EOL_TYPE;
  pBlock->nNumberOfLines = 1;  /* we'll have at least one line */
  pBlock->blockattr = blockattr;

  /*
  Detect string End-Of-Line marker type
  */
  p = strchr(pBlock->pBlock, '\r');  /* Search for end of line -- CR/LF pair */
  if (p == NULL)
  {
    /*
    Check for LF end-of-line type
    */
    p = strchr(pBlock->pBlock, '\n');

    if (p == NULL)
      goto _process_block;  /* use DEFAULT_EOL_TYPE */

    pBlock->nEOLType = LFtype;
  }
  else
  {
    /*
    CR found
    Check for CR/LF sequence first
    */
    if (*(p + 1) == '\n')
      pBlock->nEOLType = CRLFtype;
    else  /* it is CR end-of-line only */
      pBlock->nEOLType = CRtype;
  }

  /*
  Convert the lines to ASCIIZ strings.
  Two approaches:
  1. Separate and collect lines.
  2. First separate and then collect.
  Second approach is faster because the first
  requires constant realloc which is heap fragmentation
  factor. Tests proved that second approach is faster.
  */
_process_block:
  p = pBlock->pBlock;  /* PASS 1 */
  while (p != NULL)
  {
    pSentinel = p;  /* Preserve the position of the last line */
    switch (pBlock->nEOLType)
    {
      case CRLFtype:
      case CRtype:
       p = strchr(p, '\r');
       break;
      case LFtype:
       p = strchr(p, '\n');
       break;
    }
    if (p != NULL)
    {
      *p = '\0';
      ++p;  /* Skip '\0' (was CR, or LF) */
      if (pBlock->nEOLType == CRLFtype)
        ++p;  /* Skip 'LF' */
      ++pBlock->nNumberOfLines;
    }
  }

  /*
  pFile->pIndex collects the line pointers.
  The "complex" calculations in nArrayItems aim to keep
  the array size always denominated to FILE_DELTA.
  */
  TArrayInit(pBlock->pIndex, pBlock->nNumberOfLines, 1);
  if (pBlock->pIndex == NULL)
  {
    DisposeBlock(pBlock->pBlock);
    goto _dispose_pBlock;
  }

  p = pBlock->pBlock;  /* PASS 2 */
  pLn = pBlock->pIndex;
  bStop = FALSE;
  while (!bStop)
  {
    pLn->pLine = p;
    pLn->pFileBlock = pBlock->pBlock;  /* pLine is a reference inside this block */
    pLn->attr = 0;

    if (p == pSentinel)
      bStop = TRUE;

    p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
    pLn->nLen = p - pLn->pLine;  /* Calc the line length */

    ++pLn;

    ASSERT(p != NULL);
    ++p;  /* Skip '\0' (was CR, or LF) */
    if (pBlock->nEOLType == CRLFtype)
      ++p;  /* Skip 'LF' */
  }
  ASSERT(p - pBlock->pBlock <= (int)(strlen(s) + DEFAULT_EOL_SIZE));


  IncRef(pBlock->pBlock, pBlock->nNumberOfLines);  /* Update the file block reference counter */
  TArraySetCount(pBlock->pIndex, pBlock->nNumberOfLines);

  return pBlock;
}

/* ************************************************************************
   Function: InsertBlanks
   Description:
     Inserts blanks at the cursor position.
     nFromCol..nToCol marks the space to be filed.
     nLine used if set (!= -1)
*/
BOOLEAN InsertBlanks(TFile *pFile, int nLine, int nFromCol, int nToCol)
{
  TBlock *pBlock;
  int nSize;
  char *p;
  TLine *pLn;
  BOOLEAN bResult;

  ASSERT(VALID_PFILE(pFile));

  if (nFromCol == nToCol)
    return TRUE;  /* Nothing to insert */

  pBlock = AllocTBlock();
  if (pBlock == NULL)
    return FALSE;

  if (nLine == -1)
    nLine = pFile->nRow;

  nSize = CalcBlanks(nFromCol , nToCol);
  pBlock->pBlock = AllocateTBlock(nSize + DEFAULT_EOL_SIZE);
  if (pBlock->pBlock == NULL)
  {
_dispose_pBlock:
    FreeTBlock(pBlock);
    return FALSE;
  }
  p = pBlock->pBlock;
  PutBlanks(&p, nFromCol, nToCol);
  ASSERT(p - pBlock->pBlock == nSize);
  *p = '\0';

  pBlock->nEOLType = DEFAULT_EOL_TYPE;
  pBlock->nNumberOfLines = 1;  /* character block is always +1 more line */
  pBlock->blockattr = 0;

  TArrayInit(pBlock->pIndex, pBlock->nNumberOfLines, 1);
  if (pBlock->pIndex == NULL)
  {
    DisposeBlock(pBlock->pBlock);
    goto _dispose_pBlock;
  }

  p = pBlock->pBlock;
  pLn = pBlock->pIndex;

  pLn->pLine = p;
  pLn->pFileBlock = pBlock->pBlock;  /* pLine is a reference inside this block */
  pLn->attr = 0;

  p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
  pLn->nLen = p - pLn->pLine;  /* Calc the line length */

  IncRef(pBlock->pBlock, pBlock->nNumberOfLines);  /* Update the file block reference counter */
  TArraySetCount(pBlock->pIndex, pBlock->nNumberOfLines);

  bResult = InsertCharacterBlock(pFile, pBlock, nFromCol, nLine);

  DisposeABlock(&pBlock);
  return bResult;
}

/* ************************************************************************
   Function: PrepareColRowPosition
   Description:
     It may happend at the current cursor position in terms of Col and Row
     to have no text. In such a case, direct placing of a character at this position
     is impossible. Such a case is when the cursors is beyond the most right
     character of the current line. This function pads the line with blanks
     to make Col/Row position suitable for direct text insertion.
     More: Checks whether Col/Row position is not inside a tab character.
     In this case this particular region of the line is detabed.
*/
BOOLEAN PrepareColRowPosition(TFile *pFile, const TBlock *pBlockToInsert)
{
  int nLastCol;
  TLine *pLine;
  int nPos;
  int nStartCol;
  int nEndCol;
  BOOLEAN bResult;
  int nSaveCol;
  int nSaveRow;
  TBlock *pBlock;
  int nNumberOfLines;

  nNumberOfLines = 0;
  if (pBlockToInsert != NULL)
    nNumberOfLines = pBlockToInsert->nNumberOfLines;

  if (pFile->pCurPos == NULL)
  {
    if (nNumberOfLines > 1)  /* Multiline insertion, no padding needed */
    {
      /* and empty line joins <*** end of file ***> line? */
      if (GetBlockLine(pBlockToInsert, nNumberOfLines - 1)->nLen == 0)
        return TRUE;
    }
    /* 
    File is empty or inserting at the end of the file.
    It is necessary an empty line to be inserted.
    */
    nSaveCol = pFile->nCol;
    pBlock = MakeBlock("\n", 0);
    if (pBlock == NULL)
      return FALSE;
    bResult = InsertCharacterBlock(pFile, pBlock, -1, 0);
    DisposeABlock(&pBlock);
    if (!bResult)
      return FALSE;
    /* Goto the last line and restore the column position too */
    GotoColRow(pFile, nSaveCol, pFile->nNumberOfLines - 1);
  }

  pLine = GetLine(pFile, pFile->nRow);
  nLastCol = GetTabPos(pFile, pFile->nRow, pLine->nLen);
  if (pFile->nCol <= nLastCol)
  {
    /*
    Check whether current position is not inside tab area
    */
    nPos = pFile->pCurPos - pLine->pLine;
    nEndCol = GetTabPos(pFile, pFile->nRow, nPos + 1);
    if (nEndCol != pFile->nCol)
    {
      /*
      The cursor falls inside a tab area.
      Detab this position!
      */
      nStartCol = GetTabPos(pFile, pFile->nRow, nPos);
      nSaveCol = pFile->nCol;
      nSaveRow = pFile->nRow;
      bResult = DetabColumnRegion(pFile,
        pFile->nRow, nStartCol, pFile->nRow, nEndCol);
      GotoColRow(pFile, nSaveCol, nSaveRow);  /* Next operation should be here */
      return bResult;
    }
    return TRUE;  /* Nothing to do */
  }

  return InsertBlanks(pFile, -1, nLastCol, pFile->nCol);
}

/* ************************************************************************
   Function: DeleteACharacter
   Description:
     That is a simple function that deletes the character
     under the cursor.
     If the cursor is at the end of the line then
     this line and the next one are joined.
*/
void DeleteACharacter(TFile *pFile)
{
  int nCurPos;
  TLine *pLine;
  BOOLEAN bResult;
  int nUndoEntry;

  ASSERT(VALID_PFILE(pFile));

  if (!bPersistentBlocks)
    if (pFile->bBlock)
    {
      DeleteBlock(pFile);
      return;
    }

  if (pFile->pCurPos == NULL)
    return;  /* File is empty or at <*** End Of File ***> line */

  bResult = TRUE;
  nUndoEntry = UNDO_BLOCK_BEGIN();

  pLine = GetLine(pFile, pFile->nRow);
  nCurPos = pFile->pCurPos - pLine->pLine;
  ASSERT(nCurPos <= pLine->nLen);
  if (nCurPos < pLine->nLen)
  {
    bResult = DeleteCharacterBlock(pFile, pFile->nRow, nCurPos, pFile->nRow, nCurPos);
  }
  else
  {
    /*
    Join with the next line
    */
    if (!PrepareColRowPosition(pFile, NULL))
    {
      bResult = FALSE;
      goto _exit_point;
    }
    pLine = GetLine(pFile, pFile->nRow);
    nCurPos = pFile->pCurPos - pLine->pLine;
    ASSERT(nCurPos == pLine->nLen);  /* Should be at the end of line */
    bResult = DeleteCharacterBlock(pFile, pFile->nRow, nCurPos, pFile->nRow + 1, -1);
  }

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);

  return;
}

/* ************************************************************************
   Function: DeleteBlock
   Description:
     Deletes the currently marked block
*/
BOOLEAN DeleteBlock(TFile *pFile)
{
  BOOLEAN bResult;
  int nUndoEntry;

  if (!pFile->bBlock)
    return FALSE;

  bResult = TRUE;
  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (pFile->blockattr & COLUMN_BLOCK)
  {
    if (!DetabColumnRegion(pFile,
      pFile->nStartLine, pFile->nStartPos, pFile->nEndLine, pFile->nEndPos))
    {
      bResult = FALSE;
      goto _exit_point;
    }
    bResult = DeleteColumnBlock(pFile,
      pFile->nStartLine, pFile->nStartPos, pFile->nEndLine, pFile->nEndPos);

    pFile->bBlock = FALSE;
    pFile->nEndPos = pFile->nStartPos;
    if (!bPersistentBlocks)
      pFile->blockattr &= ~COLUMN_BLOCK;  /* Turn it off after the block in unmarked */
  }
  else
  {
    bResult = DeleteCharacterBlock(pFile,
      pFile->nStartLine, pFile->nStartPos, pFile->nEndLine, pFile->nEndPos);
  }

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);
  return bResult;
}

/* ************************************************************************
   Function: Cut
   Description:
     Cuts a block from pFile into pBlock
     if pBlock == NULL only deletes the current block from the file.
*/
void Cut(TFile *pFile, TBlock **pBlock)
{
  int nUndoEntry;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(pBlock != NULL);

  if (!pFile->bBlock)
    return;

  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (*pBlock != NULL)
    DisposeABlock(pBlock);

  *pBlock = MakeACopyOfTheBlock(pFile);

  DeleteBlock(pFile);

  UNDO_BLOCK_END(nUndoEntry, TRUE);
}

/* ************************************************************************
   Function: Copy
   Description:
     Copies a block from pFile into pBlock.
*/
void Copy(TFile *pFile, TBlock **pBlock)
{
  ASSERT(VALID_PFILE(pFile));
  ASSERT(pBlock != NULL);

  if (!pFile->bBlock)
    return;

  if (*pBlock != NULL)
    DisposeABlock(pBlock);

  *pBlock = MakeACopyOfTheBlock(pFile);
}

/* ************************************************************************
   Function: PrepareColRowPositionColumn
   Description:
     Prepares current cursor position to insert a column block at.
     1. Checks whether there are enough lines to put all the column block.
     Adds lines at the end of the file to make up if necessary.
     2. Detabs the column where the column block will be inserted.
*/
static BOOLEAN PrepareColRowPositionColumn(TFile *pFile, int nNumberOfLines)
{
  int nSaveCol;
  int nSaveRow;
  TBlock *pNewLineBlock;

  nSaveCol = pFile->nCol;
  nSaveRow = pFile->nRow;

  /*
  Add the necessary number of lines
  */
  if (nSaveRow + nNumberOfLines > pFile->nNumberOfLines)
  {
    pNewLineBlock = MakeBlock(DEFAULT_EOL, 0);
    if (pNewLineBlock == NULL)
      return FALSE;
    GotoColRow(pFile, 0, pFile->nNumberOfLines);
    while (nSaveRow + nNumberOfLines > pFile->nNumberOfLines)
    {
      if (!InsertCharacterBlock(pFile, pNewLineBlock, -1, -1))
        return FALSE;
    }
    DisposeABlock(&pNewLineBlock);
  }

  /*
  Detab the column where the block is about to be inserted
  */
  if (!DetabColumnRegion(pFile,
      nSaveRow, nSaveCol, nSaveRow + nNumberOfLines - 1, nSaveCol))
    return FALSE;

  GotoColRow(pFile, nSaveCol, nSaveRow);
  return TRUE;
}

/* ************************************************************************
   Function: Paste
   Description:
     Pastes a block from pBlock into pFile.
*/
BOOLEAN Paste(TFile *pFile, const TBlock *pBlock)
{
  int nUndoEntry;
  BOOLEAN bResult;

  ASSERT(VALID_PFILE(pFile));

  if (pBlock == NULL)
    return TRUE;

  ASSERT(VALID_PBLOCK(pBlock));

  bResult = TRUE;
  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (!bPersistentBlocks)
    DeleteBlock(pFile);

  if (pBlock->blockattr & COLUMN_BLOCK)
  {
    /*
    Make detab of all the lines where the block will be inserted
    Add lines at the end of the file if no enough lines to insert
    all the column block.
    */
    if (!PrepareColRowPositionColumn(pFile, pBlock->nNumberOfLines))
    {
      bResult = FALSE;
      goto _exit_point;
    }
    bResult = InsertColumnBlock(pFile, pBlock);
    ASSERT(pFile->pCurPos != NULL);
    ASSERT(INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos) >= 0);
  }
  else
  {
    /* Ensure characters under the current column position */
    if (!PrepareColRowPosition(pFile, pBlock))
    {
      bResult = FALSE;
      goto _exit_point;
    }
    bResult = InsertCharacterBlock(pFile, pBlock, -1 ,-1);
  }

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);
  return bResult;
}

/* ************************************************************************
   Function: CalcBlockIndent
   Description:
*/
static int FirstNonBlank(const char *ln)
{
  const char *p;

  p = ln;
  while (*p)
    if (!(*p == ' ' || *p == '\t'))
      break;
    else
      ++p;

  return p - ln;
}

/* ************************************************************************
   Function: CalcBlockIndent
   Description:
*/
static int CalcBlockIndent(const TBlock *pBlock)
{
  int nIndent;
  int x;
  int i;
  TLine *pLn;

  nIndent = -1;
  pLn = pBlock->pIndex;
  for (i = 0; i < pBlock->nNumberOfLines; ++i)
  {
    if (pLn->nLen > 0)
    {
      x = FirstNonBlank(pLn->pLine);
      if (nIndent == -1)
        nIndent = x;
      if (x < nIndent)
        nIndent = x;
    }
    ++pLn;
  }
  return nIndent;
}

/* ************************************************************************
   Function: PasteAndIndent
   Description:
     Pastes a block from pBlock into pFile.
     Then attempts to align the block depending on the indent
     of the lines where it was pasted.
*/
BOOLEAN PasteAndIndent(TFile *pFile, const TBlock *pBlock)
{
  int nUndoEntry;
  BOOLEAN bResult;
  TEditInterf stInterf;
  TCalcIndentProc pfnCalcIndent;
  int nDestIndent;
  int nBlockIndent;
  int nPos1;
  int nLn1;
  int nPos2;
  int nLn2;

  ASSERT(VALID_PFILE(pFile));

  bResult = TRUE;
  nUndoEntry = UNDO_BLOCK_BEGIN();

  pfnCalcIndent = GetCalcIndentProc(pFile->nType);
  if (pfnCalcIndent == NULL)
  {
    bResult = Paste(pFile, pBlock);
    goto _exit_point;
  }

  ProduceEditInterf(&stInterf, pFile);
  nDestIndent = pfnCalcIndent(&stInterf);
  if (nDestIndent < 0)
  {
    bResult = Paste(pFile, pBlock);
    goto _exit_point;
  }

  nPos1 = INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos);
  nLn1 = pFile->nRow;
  if (!Paste(pFile, pBlock))
  {
    bResult = FALSE;
    goto _exit_point;
  }
  nPos2 = INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos);
  nLn2 = pFile->nRow;

  nBlockIndent = CalcBlockIndent(pBlock);
  if (nBlockIndent <= 0)
    goto _exit_point;

  GotoPosRow(pFile, nPos1, nLn1);
  MarkBlockBegin(pFile);
  GotoPosRow(pFile, nPos2, nLn2);
  MarkBlockEnd(pFile);

  if (nDestIndent > nBlockIndent)
    IndentBlock(pFile, nDestIndent - nBlockIndent);
  else
    if (nDestIndent != nBlockIndent)
      UnindentBlock(pFile, nBlockIndent - nDestIndent);

  ToggleBlockHide(pFile);  /* Hide the block */

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);
  return bResult;
}

/* ************************************************************************
   Function: ReplaceText
   Description:
     A wrap function of ReplaceTextPrim().
     Adds undo/redo log of the operation.
*/
static BOOLEAN ReplaceTextWrap(TFile *pFile, const char *pText)
{
  int nUndoEntry;
  BOOLEAN bResult;
  TBlock *pBlock;
  int nStartPos;
  int nEndPos;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(pFile->pCurPos != NULL);
  ASSERT(pText != NULL);
  ASSERT(pText[0] != '\0');

  bResult = TRUE;
  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (!PrepareColRowPosition(pFile, NULL))  /* If inside tab area */
  {
    bResult = FALSE;
    goto _exit_point;
  }

  if (!AddUndoRecord(pFile, acREPLACE, FALSE))
  {
    bResult = FALSE;
    goto _exit_point;
  }

  /* below: Ensure there's enough characters after pCurPos to put all from pText */
  ASSERT(strchr(pFile->pCurPos, '\0') - pFile->pCurPos >= (int)strlen(pText));

  nStartPos = INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos);
  nEndPos = nStartPos + strlen(pText);
  pBlock = MakeACopyOfBlock(pFile, pFile->nRow, pFile->nRow, nStartPos, nEndPos - 1, 0);
  if (pBlock == NULL)
  {
    bResult = FALSE;
    goto _exit_point;
  }
  ReplaceTextPrim(pFile, pText, NULL);
  RecordUndoData(pFile, pBlock, pFile->nRow, nStartPos, pFile->nRow, nEndPos - 1);

  pFile->bChanged = TRUE;
  pFile->bRecoveryStored = FALSE;

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);
  return bResult;
}

/* ************************************************************************
   Function: Overwrite
   Description:
     Pastes a block from pBlock into pFile in overwrite mode.
     This should be a single character block!
*/
void Overwrite(TFile *pFile, const TBlock *pBlock)
{
  int nUndoEntry;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PBLOCK(pBlock));
  ASSERT((pBlock->blockattr & COLUMN_BLOCK) == 0);
  ASSERT(pBlock->nNumberOfLines == 1);

  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (!bPersistentBlocks)
    DeleteBlock(pFile);

  if (pFile->pCurPos != NULL && *(pFile->pCurPos) != '\0')
    ReplaceTextWrap(pFile, GetBlockLineText(pBlock, 0));
  else
    Paste(pFile, pBlock);

  pFile->bUpdateLine = TRUE;

  UNDO_BLOCK_END(nUndoEntry, TRUE);
}

/* ************************************************************************
   Function: IndentBlock
   Description:
*/
static void _IndentBlock(TFile *pFile)
{
  int i;
  int nStartLine;
  int nEndLine;
  TLine *pLine;
  char *p;
  int nPos;
  int nCol;
  int nLastTabPos;
  int nLastTabCol;
  int nUndoEntry;
  BOOLEAN bResult;
  int nStart;
  int nEnd;
  int nSaveCol;
  int nSaveRow;
  int nIndent;

  ASSERT(VALID_PFILE(pFile));
  nIndent = 1;

  if (!pFile->bBlock)
    return;

  nUndoEntry = UNDO_BLOCK_BEGIN();
  bResult = TRUE;

  nStartLine = pFile->nStartLine;
  nEndLine = pFile->nEndLine;
  if (pFile->nEndPos == -1)
    --nEndLine;
  ASSERT(nEndLine >= 0);

  nSaveCol = pFile->nCol;
  nSaveRow = pFile->nRow;

  for (i = nStartLine; i <= nEndLine; ++i)
  {
    pLine = GetLine(pFile, i);

    /*
    Search for the first non-blank. Calc the tab position.
    */
    nCol = 0;
    nLastTabPos = -1;
    nLastTabCol = 0;
    for (p = pLine->pLine; *p; ++p)
    {
      if (!isspace(*p))
        break;
      if (*p == '\t')  /* Tab char */
      {
        nCol = CalcTab(nCol);
        nLastTabPos = p - pLine->pLine;
        nLastTabCol = nCol;
      }
      else
        ++nCol;
    }

    if (*p == '\0')
      continue;
    nPos = p - pLine->pLine;

    /*
    Decisions:
    1. If bOptimalFill or bUseTabs are FALSE simple amount of spaces will
    accomplish the task;
    -or-
    2. We need to optimize the leading spaces area after adding nIndent
    spaces, by replacing spaces ' ' with tabs '\t'
    */
    if (bOptimalFill == FALSE || bUseTabs == FALSE)
    {
      if (!InsertBlanks(pFile, i, nCol, nCol + nIndent))
      {
        bResult = FALSE;
        goto _exit_point;
      }
      continue;
    }
    nStart = nCol;
    nEnd = nCol + nIndent;
    if (nCol + nIndent >= nLastTabCol + nTabSize)
    {
      /*
      We need to optimize the spaces area.
      First remove the spaces, then to be replaced by tabs.
      */
      if (nLastTabPos == -1)
      {
        bResult = DeleteCharacterBlock(pFile, i, 0, i, nPos - 1);
        nStart = 0;
      }
      else
      {
        bResult = DeleteCharacterBlock(pFile, i, nLastTabPos + 1, i, nPos - 1);
        nStart = nLastTabCol;
      }
    }
    if (!bResult)
      goto _exit_point;
    if (!InsertBlanks(pFile, i, nStart, nEnd))
    {
      bResult = FALSE;
      goto _exit_point;
    }
  }

  /*
  Mark the entire area that was shifted
  */
  pFile->bBlock = TRUE;
  pFile->blockattr = 0;
  pFile->nStartLine = nStartLine;
  pFile->nStartPos = 0;
  pFile->nEndLine = nEndLine + 1;
  pFile->nEndPos = -1;

  pFile->bUpdatePage = TRUE;

  GotoColRow(pFile, nSaveCol, nSaveRow);

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);
}

/* ************************************************************************
   Function: IndentBlock
   Description:
*/
void IndentBlock(TFile *pFile, int nIndent)
{
  while (nIndent-- > 0)
    _IndentBlock(pFile);
}

/* ************************************************************************
   Function: UnindentBlock
   Description:
*/
static void _UnindentBlock(TFile *pFile)
{
  int i;
  int nStartLine;
  int nEndLine;
  TLine *pLine;
  char *p;
  int nPos;
  int nCol;
  int nUndoEntry;
  BOOLEAN bResult;
  int nSaveCol;
  int nSaveRow;

  ASSERT(VALID_PFILE(pFile));

  if (!pFile->bBlock)
    return;

  nUndoEntry = UNDO_BLOCK_BEGIN();
  bResult = TRUE;

  nStartLine = pFile->nStartLine;
  nEndLine = pFile->nEndLine;
  if (pFile->nEndPos == -1)
    --nEndLine;
  ASSERT(nEndLine >= 0);

  nSaveCol = pFile->nCol;
  nSaveRow = pFile->nRow;

  for (i = nStartLine; i <= nEndLine; ++i)
  {
    pLine = GetLine(pFile, i);

    /*
    Search for the first non-blank. Calc the tab position.
    */
    nCol = 0;
    for (p = pLine->pLine; *p; ++p)
    {
      if (!isspace(*p))
        break;
      if (*p == '\t')  /* Tab char */
        nCol = CalcTab(nCol);
      else
        ++nCol;
    }

    if (*p == '\0')
      continue;

    if (p == pLine->pLine)
      continue;  /* Nowhere to unindent */

    /*
    Detab the region of one character.
    We can use the DetabColumnRegion() function by specifying only
    a single character as a region to be detabed.
    */
    bResult = DetabColumnRegion(pFile, i, nCol - 1, i, nCol - 1);
    if (!bResult)
      goto _exit_point;

    /*
    We need to walk again from start of the line up to the first
    non-blank position in order to have the character
    position where to delete
    */
    pLine = GetLine(pFile, i);
    for (p = pLine->pLine; *p; ++p)
    {
      if (!isspace(*p))
        break;
    }
    ASSERT(*p != '\0');
    ASSERT(p != pLine->pLine);

    nPos = p - pLine->pLine;
    ASSERT(nPos < pLine->nLen);
    bResult = DeleteCharacterBlock(pFile, i, nPos - 1, i, nPos - 1);
    if (!bResult)
      goto _exit_point;
  }

  /*
  Mark the entire area that was shifted
  */
  pFile->bBlock = TRUE;
  pFile->blockattr = 0;
  pFile->nStartLine = nStartLine;
  pFile->nStartPos = 0;
  pFile->nEndLine = nEndLine + 1;
  pFile->nEndPos = -1;

  pFile->bUpdatePage = TRUE;

  GotoColRow(pFile, nSaveCol, nSaveRow);

_exit_point:
  UNDO_BLOCK_END(nUndoEntry, bResult);
}

/* ************************************************************************
   Function: UnindentBlock
   Description:
*/
void UnindentBlock(TFile *pFile, int nUnindent)
{
  while (nUnindent-- > 0)
    _UnindentBlock(pFile);
}

/* ************************************************************************
   Function: AddCharLine
   Description:
     This function is used for in-memory file operations when a lot of
     lines are generated and MakeBlock()-Paste() sequence of operations
     is generating too fragmented file image.
     This function operated on list of blocks each of 4kbytes size.
*/
BOOLEAN AddCharLine(TFile *pFile, const char *psLine)
{
  int nBlockSize;
  int nBlockFree;
  int nNewSize;
  int nLineLen;
  char *pLinePos;
  char *pBlock;
  TLine stLine;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(psLine != NULL);

  if (pFile->pIndex == NULL)
  {
    /*
    Empty file -- prapare an Index
    */
    TArrayInit(pFile->pIndex, 1, FILE_DELTA);
    if (pFile->pIndex == NULL)
      return FALSE;
  }

  /*
  Add a line.
  Check whether there is a place in the current TFileBlock to add the
  line to.
  */
  nLineLen = strlen(psLine);
  nBlockFree = 0;
  if (!IS_LIST_EMPTY(&pFile->blist))
  {
    pBlock = GetLastBlock(&pFile->blist);
    nBlockFree = GetTBlockFree(pBlock);
  }
  if (nBlockFree < nLineLen + 1)
  {
    nNewSize = 4096;
    if (nLineLen > nNewSize)
      nNewSize = nLineLen + 1;
    if (!AddBlock(&pFile->blist, nNewSize))
      return FALSE;
  }
  /* There is a block that can accomodate the new line */
  pBlock = GetLastBlock(&pFile->blist);
  nBlockSize = GetTBlockSize(pBlock);
  nBlockFree = GetTBlockFree(pBlock);
  ASSERT(nBlockFree <= nBlockSize);
  ASSERT(nBlockFree >= nLineLen + 1);
  pLinePos = &pBlock[nBlockSize - nBlockFree];
  strcpy(pLinePos, psLine);
  nBlockFree -= nLineLen + 1;
  SetTBlockFree(pBlock, nBlockFree);

  stLine.pLine = pLinePos;
  stLine.attr = 0;
  stLine.nLen = nLineLen;
  stLine.pFileBlock = pBlock;
  TArrayAdd(pFile->pIndex, stLine);
  if (!TArrayStatus(pFile->pIndex))  /* Failed to insert the lines */
  {
    TArrayClearStatus(pFile->pIndex);
    return FALSE;
  }

  IncRef(pBlock, 1);  /* Update the TFileBlock reference counter */
  pFile->nNumberOfLines++;
  GotoColRow(pFile, 0, pFile->nNumberOfLines);  /* text inserted at end of file */

  return TRUE;
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

