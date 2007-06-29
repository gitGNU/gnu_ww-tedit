/*

File: clips.c
Description:
  I store some obsolete function sources here.

*/

/* ************************************************************************
   Function: MakeColumnBlockFootprint
   Description:
     Creates a copy of an column area that is about to be deleted.
     The resulting TBlock is close to nature to the FitColumnBlock()
     block pattern.
     The are should be de-tabed before calling MakeColumnBlockFootprint()
*/
static TBlock *MakeColumnBlockFootprint(const TFile *pFile,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos)
{
  int nBlockSize;
  int nLineLen;
  int nLastCol;
  char *p;
  char *d;
  int nCol;
  char *pBlankSection;
  int nBlockWidth;
  int i;
  TBlock *pNewBlock;
  int nNumberOfLines;
  TLine *pNewBlockLine;
  char *pCopyFrom;
  int nToCopy;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nStartLine >= 0);
  ASSERT(nEndLine >= nStartLine);
  ASSERT(nEndLine < pFile->nNumberOfLines);
  ASSERT(nStartPos >= 0);
  ASSERT(nEndPos >= 0);
  ASSERT(nEndPos >= nStartPos);

  /*
  Calc the size of the pattern block.
  */
  nBlockSize = 0;
  for (i = nStartLine; i <= nEndLine; ++i)
  {
    nLineLen = GetLine(pFile, i)->nLen;

    nLastCol = GetTabPos(pFile, i, nLineLen) - 1;

    if (nLastCol > nStartPos)
    {
      /*
      There is a text in the column block region in this line
      */

      if (nLastCol > nEndPos)
        nBlockWidth = nEndPos - nStartPos + 1;
      else
        nBlockWidth = nLastCol - nStartPos + 1;

      /*
      Add the blank space that is left to nStartPos,
      if removing this peace of text will result in leaving
      white spaces at the most right of the line.
      */
      if (nLastCol <= nEndPos && bRemoveTrailingBlanks)
      {
        p = GetLineText(pFile, i);
        pBlankSection = NULL;  /* Still no section */
        nCol = 0;
        while (*p)
        {
          if (nCol >= nStartPos)  /* We get to the desired column */
          {
            /* The section should have be detabed! */
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

        if (pBlankSection != NULL)
          nBlockWidth += p - pBlankSection;
      }

      nBlockSize += nBlockWidth;
    }

    nBlockSize += DEFAULT_EOL_SIZE;
  }

  /*
  Allocate a block to put the column block pattern into
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

  nNumberOfLines = nEndLine - nStartLine + 1;
  pNewBlock->blockattr = COLUMN_BLOCK;
  pNewBlock->nEOLType = DEFAULT_EOL_TYPE;
  pNewBlock->nNumberOfLines = nNumberOfLines;
  TArrayInit(pNewBlock->pIndex, nNumberOfLines, FILE_DELTA);
  if (pNewBlock->pIndex == NULL)
  {
    DisposeBlock(pNewBlock->pBlock);
    goto _dispose_pBlock;
  }
  pNewBlockLine = pNewBlock->pIndex;
  p = pNewBlock->pBlock;
  memset(pNewBlock->pBlock, '\0', nBlockSize);

  /*
  Walk along all the lines contained the column block
  and copy to pNewBlock.
  */
  for (i = nStartLine; i <= nEndLine; ++i)
  {
    pCopyFrom = NULL;
    nToCopy = -1;

    nLineLen = GetLine(pFile, i)->nLen;

    nLastCol = GetTabPos(pFile, i, nLineLen) - 1;

    pNewBlockLine->pLine = p;
    pNewBlockLine->pFileBlock = pNewBlock->pBlock;
    pNewBlockLine->attr = 0;

    if (nLastCol > nStartPos)
    {
      /*
      There is a text in the column block region in this line
      */
      if (nLastCol > nEndPos)
        nToCopy = nEndPos - nStartPos + 1;
      else
        nToCopy = nLastCol - nStartPos + 1;

      d = GetLineText(pFile, i);
      pBlankSection = NULL;
      nCol = 0;
      while (*d)
      {
        if (nCol >= nStartPos)  /* We get to the desired column */
        {
          /* The section should have be detabed! */
          ASSERT(nCol == nStartPos);
          pCopyFrom = d;
          break;
        }

        if (isspace(*d))
        {
          if (pBlankSection == NULL)
            pBlankSection = d;  /* Mark the start of a blank section */
        }
        else
          pBlankSection = NULL;  /* We come across a non-blank character */

        if (*d == '\t')  /* Tab char */
          nCol = CalcTab(nCol);
        else
          nCol++;
        d++;
      }

      /*
      Add the blank space that is left to nStartPos,
      if removing this peace of text will result in leaving
      white spaces at the most right of the line.
      */
      if (nLastCol <= nEndPos && bRemoveTrailingBlanks)
      {
        nToCopy += pCopyFrom - pBlankSection;  /* Number of blanks */
        pCopyFrom = pBlankSection;
      }
    } /* if (nLastCol > nStartPos) -- if ther're characters for this line */

    if (pCopyFrom != NULL)
    {
      strncpy(p, pCopyFrom, nToCopy);  /* Extract the text of column block */
      p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
    }

    pNewBlockLine->nLen = p - pNewBlockLine->pLine;  /* Calc the line length */
    p += DEFAULT_EOL_SIZE;

    ++pNewBlockLine;
    ASSERT(p != NULL);
  }

  IncRef(pNewBlock->pBlock, nNumberOfLines);  /* Update the file block reference counter */
  TArraySetCount(pNewBlock->pIndex, nNumberOfLines);

  return pNewBlock;
}

