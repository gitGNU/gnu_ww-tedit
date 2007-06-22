/*

File: file.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Functions concerning file storage and manipulation.

*/

#include "global.h"
#include "clist.h"
#include "heapg.h"
#include "maxpath.h"
#include "path.h"
#include "tblocks.h"
#include "memory.h"
#include "l1opt.h"
#include "l1def.h"
#include "file.h"
#include "undo.h"

/* ************************************************************************
   Function: InitEmptyFile
   Description:
     Initial setup of a file structure.
*/
void InitEmptyFile(TFile *pFile)
{
  static int nID;

  ASSERT(pFile != NULL);

  #ifdef _DEBUG
  pFile->MagicByte = FILE_MAGIC;
  #endif

  INITIALIZE_LIST_HEAD(&pFile->blist);
  pFile->pIndex = NULL;

  pFile->nEOLType = -1;   /* undetected */
  pFile->nCR = -1;
  pFile->nLF = -1;
  pFile->nCRLF = -1;
  pFile->nZero = -1;
  pFile->nEOLTypeDisk = -1;  /* still not stored */

  pFile->sFileName[0] = '\0';
  pFile->sTitle[0] = '\0';
  pFile->nType = -1;
  pFile->sRecoveryFileName[0] = '\0';
  pFile->nCopy = 0;
  pFile->sMsg[0] = '\0';
  pFile->nFileSize = 0;
  pFile->nRow = 0;
  pFile->nCol = 0;
  pFile->x = 0;
  pFile->y = 0;
  pFile->lnattr = 0;
  pFile->bShowBlockCursor = FALSE;
  pFile->pCurPos = NULL;  /* no current position */
  pFile->nTopLine = 0;
  pFile->nWrtEdge = 0;
  pFile->nNumberOfLines = 0;
  pFile->bBlock = FALSE;  /* no	block */
  pFile->nStartLine = -1;  /* invalid */
  pFile->nEndLine = -1;  /* invalid */
  pFile->nStartPos = -1;  /* invalid */
  pFile->nEndPos = -2;  /* -1 is valid */
  pFile->blockattr = 0;
  pFile->bChanged = FALSE;
  pFile->bRecoveryStored = FALSE;
  pFile->bFileNameChanged = FALSE;
  pFile->bForceNewRecoveryFile = FALSE;
  pFile->bForceReadOnly = FALSE;
  pFile->bReadOnly = FALSE;
  pFile->bNew = FALSE;

  pFile->bDisplayChanged = TRUE;
  pFile->bDisplayColumnMode = TRUE;
  pFile->bDisplayInsertMode = TRUE;
  pFile->bDisplayReadOnly = TRUE;

  pFile->pUndoIndex = NULL;
  pFile->nUndoLevel = 0;
  pFile->nNumberOfRecords = 0;
  pFile->nUndoIDCounter = 0;

  pFile->LastWriteTime.year = -1;
  pFile->LastWriteTime.month = -1;
  pFile->LastWriteTime.day = -1;
  pFile->LastWriteTime.hour = -1;
  pFile->LastWriteTime.min = -1;
  pFile->LastWriteTime.sec = -1;

  pFile->nExpandCol = -1;
  pFile->nExpandRow = -1;
  pFile->nLastExtendCol = -1;
  pFile->nLastExtendRow = -1;
  pFile->nExtendAncorCol = -1;
  pFile->nExtendAncorRow = -1;
  pFile->bPreserveSelection = FALSE;

  pFile->nPrevCol = -1;
  pFile->nPrevRow = -1;

  pFile->nRecStatFileSize = -1;
  pFile->nRecStatMonth = -1;
  pFile->nRecStatDay = -1;
  pFile->nRecStatYear = -1;
  pFile->nRecStatHour = -1;
  pFile->nRecStatMin = -1;
  pFile->nRecStatSec = -1;

  pFile->sEndOfFile = (char *)sEndOfFile;

  pFile->nID = ++nID;

  pFile->pBMSetFuncNames = NULL;
  pFile->pBMFuncFindCtx = NULL;

  pFile->bHighlighAreas = FALSE;
  memset(pFile->hlightareas, 0, sizeof(pFile->hlightareas));

  pFile->sTooltipBuf = NULL;
  pFile->nTooltipBufSize = 0;
  pFile->bTooltipIsTop = FALSE;
}

/* ************************************************************************
   Function: LoadFilePrim
   Description:
     Loads a file. Basic level release.
     Gets file size, allocates a block in and adds it to blist, reads a file
     in the allocated block, renders the file to point out the lines.
   Returns:
     0 - Load OK
     2 - file doesn't exist
     3 - no memory for the file
     4 - invalid path
     5 - io error
*/
int LoadFilePrim(TFile *pFile)
{
  FILE *f;
  int nExitCode;
  char *p;
  char *pSentinel;
  TLine *pLine;
  struct stat statbuf;
  int nRead;
  char *pBlock;
  int nSizeOfCRLines;
  int nSizeOfLFLines;
  int nSizeOfCRLFLines;
  int nSizeOfZeroLines;
  int n;
  char *pBlockR;
  int nMarkSize;
  char *p2;
  int nNumberOfLines;
  BOOLEAN bNonuniformEOL;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(pFile->sFileName[0] != '\0');

  nExitCode = 0;  /* Assume no error */
  pBlockR = NULL;
  bNonuniformEOL = FALSE;

  if (stat(pFile->sFileName, &statbuf) != 0)
  {
    if (!IsValidFileName(pFile->sFileName))
    {
      nExitCode = 4;
      goto _exit_point;
    }
    /* Create new file (as the path is valid) */
    nExitCode = 2;
    goto _exit_point;
  }

  /* Check for read only */
  pFile->bReadOnly = FALSE;
  #ifdef __GNUC__
  if ((statbuf.st_mode & S_IWUSR) == 0)
    pFile->bReadOnly = TRUE;
  #else
  if ((statbuf.st_mode & S_IWRITE) == 0)
    pFile->bReadOnly = TRUE;
  #endif

  if (statbuf.st_size == 0)
  {
    pFile->nEOLType = DEFAULT_EOL_TYPE;
    goto _exit_point;  /* Empty file */
  }

  f = fopen(pFile->sFileName, READ_BINARY_FILE);
  if (f == NULL)
  {
    nExitCode = 5;
    goto _exit_point;
  }

  pFile->nFileSize = statbuf.st_size;

  if (!AddBlock(&pFile->blist, pFile->nFileSize + 3))  /* +3 for ASCIIZ/EOL */
  {
    nExitCode = 3;
    goto _exit_point;
  }

  pBlock = GetLastBlock(&pFile->blist);
  nRead = fread(pBlock, 1, pFile->nFileSize, f);
  fclose(f);

  if (nRead != pFile->nFileSize)
  {
    nExitCode = 5;
_dispose_pblock:
    DisposeBlock(pBlock);
    if (pBlockR != NULL)
      DisposeBlock(pBlockR);
    goto _exit_point;
  }

  /*
  Make the whole file a single ASCIIZ string.
  */
  pBlock[pFile->nFileSize] = '\0';
  pBlock[pFile->nFileSize + 1] = '\0';
  pBlock[pFile->nFileSize + 2] = '\0';

  /*
  Scan through all the file. Count the number
  of the different end-of-line markers.
  */
  p = pBlock;
  pFile->nNumberOfLines = 0;
  nNumberOfLines = 0;
  pFile->nCR = 0;
  pFile->nLF = 0;
  pFile->nCRLF = 0;
  pFile->nZero = 0;
  nSizeOfCRLines = 0;
  nSizeOfLFLines = 0;
  nSizeOfCRLFLines = 0;
  nSizeOfZeroLines = 0;  /* account for occasional zeros inside the file */
  pSentinel = pBlock;  /* Where a line started */
  while (p - pBlock < pFile->nFileSize)
  {
    if (*p == '\0')
    {
      nSizeOfZeroLines += p - pSentinel;
      ++p;
      pSentinel = p;
      ++nNumberOfLines;
      ++pFile->nZero;
      continue;
    }
    if (*p == '\r')
    {
      ++nNumberOfLines;
      if (*(p + 1) == '\n')
      {
        nSizeOfCRLFLines += p - pSentinel;
        ++p;
        ++p;
        pSentinel = p;
        ++pFile->nCRLF;
        continue;
      }
      nSizeOfCRLines += p - pSentinel;
      ++pFile->nCR;
      ++p;
      pSentinel = p;
      continue;
    }
    if (*p == '\n')
    {
      nSizeOfLFLines += p - pSentinel;
      ++nNumberOfLines;
      ++pFile->nLF;
      ++p;
      pSentinel = p;
      continue;
    }
    ++p;
  }

  /*
  Determine the End-Of-Line type of this file
  considering the collected information.
  */
  ASSERT(pFile->nEOLType == -1);  /* Should be still undetected */
  if (pFile->nCR > pFile->nLF)
  {
    if (pFile->nCR > pFile->nCRLF)
      pFile->nEOLType = CRtype;
    if (pFile->nCRLF > pFile->nLF)
      pFile->nEOLType = CRLFtype;
  }
  else
  {
    /* nLF >= nCR */
    if (pFile->nLF > pFile->nCRLF)
      pFile->nEOLType = LFtype;
    if (pFile->nCRLF > pFile->nLF)  /* this checks whether both may be 0 */
      pFile->nEOLType = CRLFtype;
  }
  if (pFile->nEOLType == -1)  /* No single end-of-line marker in the file */
    pFile->nEOLType = DEFAULT_EOL_TYPE;  /* Use the default for the OS */

  /*
  Separate the file in ASCIIZ lines.
  */
  if (pFile->nEOLType == CRLFtype)
  {
    /*
    If the filetype is CRLFType and we have lines that
    are marked by single-byte end-of-line markers, we need to allocate
    a separate buffer where to place those lines formated to
    allow 2-byte end-of-line marker.
    */
    if (pFile->nCR > 0 || pFile->nLF > 0 || pFile->nZero > 0)
    {
      bNonuniformEOL = TRUE;
      n = pFile->nCR + pFile->nLF + pFile->nZero;  /* lines * 2 to accomodate CRLF marker */
      if (!AddBlock(&pFile->blist, nSizeOfCRLines + nSizeOfLFLines + nSizeOfZeroLines + n * 2))
      {
        nExitCode = 3;
	goto _dispose_pblock;
      }
      pBlockR = GetLastBlock(&pFile->blist);
    }
  }
  else
  {
    /*
    If we have single-character end-of-line marker as default for the file,
    we need to allocate a separate buffer where to put the CR/LF lines
    (if there are such lines) where they are to be cut to accomodate single
    character end-of-line markers.
    */
    ASSERT(pFile->nEOLType == CRtype || pFile->nEOLType == LFtype);
    if (pFile->nCRLF > 0)
    {
      bNonuniformEOL = TRUE;
      if (!AddBlock(&pFile->blist, nSizeOfCRLFLines + pFile->nCRLF * 2))
      {
        nExitCode = 3;
        goto _dispose_pblock;
      }
      pBlockR = GetLastBlock(&pFile->blist);
    }
  }

  TArrayInit(pFile->pIndex, nNumberOfLines, FILE_DELTA);
  if (pFile->pIndex == NULL)
  {
    nExitCode = 3;
    goto _dispose_pblock;
  }

  pSentinel = pBlock;  /* Where a line started */
  p = pBlock;  /* Start the walk again from the start of the block */
  p2 = pBlockR;
  nMarkSize = 1;
  if (pFile->nEOLType == CRLFtype)
    nMarkSize = 2;
  pLine = pFile->pIndex;
  pFile->nNumberOfLines = nNumberOfLines;
  while (p - pBlock < pFile->nFileSize)
  {
    if (*p == '\0')
    {
      /* Move this line in pBlockR */
      #if _DEBUG
      nSizeOfZeroLines -= p - pSentinel;
      --nNumberOfLines;
      #endif
_process_single_char_eol:
      if (pFile->nEOLType == CRtype || pFile->nEOLType == LFtype)
      {
        pLine->pLine = pSentinel;
        pLine->pFileBlock = pBlock;  /* pLine is a reference inside this block */
        IncRef(pBlock, 1);  /* Update the file block reference counter */
        pLine->attr = 0;
        pLine->nLen = p - pSentinel;  /* Calc the line length */
        ++pLine;
        *p = '\0';
        ++p;  /* Now points the start of the next line (in pBlock) */
        pSentinel = p;
      }
      else
      {
        /* this is a CR marked line in a file that uses CR/LF
        characters pair as an end-of-line marker */
        ASSERT(pFile->nEOLType == CRLFtype);
        memcpy(p2, pSentinel, p - pSentinel);
        pLine->pLine = p2;
        pLine->pFileBlock = pBlockR;  /* pLine is a reference inside this block */
        IncRef(pBlockR, 1);  /* Update the file block reference counter */
        pLine->attr = 0;
        pLine->nLen = p - pSentinel;
        ++pLine;
        p2 += p - pSentinel;
        *p2 = '\0';
        p2 += nMarkSize;  /* Place to accomodate an end-of-line marker */
        ++p;  /* Now points the start of the next line (in pBlock) */
        pSentinel = p;
      }
      continue;
    }
    if (*p == '\r')
    {
      #ifdef _DEBUG
      --nNumberOfLines;
      #endif
      if (*(p + 1) == '\n')
      {
        #ifdef _DEBUG
        nSizeOfCRLFLines -= p - pSentinel;
        #endif
        if (pFile->nEOLType == CRLFtype)
        {
          pLine->pLine = pSentinel;
          pLine->pFileBlock = pBlock;  /* pLine is a reference inside this block */
          IncRef(pBlock, 1);  /* Update the file block reference counter */
          pLine->attr = 0;
          pLine->nLen = p - pSentinel;  /* Calc the line length */
          ++pLine;
          *p = '\0';
          /* move forward the marker in the file */
          ++p;
          ++p;  /* Now points the start of the next line (in pBlock) */
          pSentinel = p;
        }
        else
        {
          /* this is a CR/LF marked line in a file that is of a single
          character end-of-line marker */
          ASSERT(nMarkSize == 1);
          memcpy(p2, pSentinel, p - pSentinel);
          pLine->pLine = p2;
          pLine->pFileBlock = pBlockR;  /* pLine is a reference inside this block */
          IncRef(pBlockR, 1);  /* Update the file block reference counter */
          pLine->attr = 0;
          pLine->nLen = p - pSentinel;
          ++pLine;
          p2 += p - pSentinel;
          *p2 = '\0';
          p2 += nMarkSize;  /* Place to accomodate an end-of-line marker */
          ++p;
          ++p;  /* Now points the start of the next line (in pBlock) */
          pSentinel = p;
        }
        continue;
      }
      #ifdef _DEBUG
      nSizeOfCRLines -= p - pSentinel;
      #endif
      goto _process_single_char_eol;
    }
    if (*p == '\n')
    {
      #ifdef _DEBUG
      nSizeOfLFLines -= p - pSentinel;
      --nNumberOfLines;
      #endif
      goto _process_single_char_eol;
    }
    ++p;
    if (p - pBlock == pFile->nFileSize)
    {
      pLine->pLine = pSentinel;
      pLine->pFileBlock = pBlock;  /* pLine is a reference inside this block */
      IncRef(pBlock, 1);  /* Update the file block reference counter */
      pLine->attr = 0;
      pLine->nLen = p - pSentinel;  /* Calc the line length */
      ++pLine;
      ASSERT(*p == '\0');  /* First of those 3 bytes we added at the end of pBlock */
      ++pFile->nNumberOfLines;
      break;
    }
  }
  ASSERT(nNumberOfLines == 0);
  ASSERT(nSizeOfZeroLines == 0);
  ASSERT(nSizeOfCRLines == 0);
  ASSERT(nSizeOfLFLines == 0);
  ASSERT(nSizeOfCRLFLines == 0);
  TArraySetCount(pFile->pIndex, pFile->nNumberOfLines);

  if (bNonuniformEOL)
  {
    ASSERT(pBlockR != NULL);
    strcpy(pFile->sMsg, sNonuniformEOL);
  }
_exit_point:
  return nExitCode;
}

/* ************************************************************************
   Function: DisposeFile
   Description:
     Disposes all the file allocations from the memory.
*/
void DisposeFile(TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));

  if (pFile->sTooltipBuf != NULL)
    s_free(pFile->sTooltipBuf);

  DisposeBlockList(&pFile->blist);
  if (pFile->pIndex != NULL)
    TArrayDispose(pFile->pIndex);

  DisposeUndoIndexData(pFile);

  #ifdef _DEBUG
  pFile->MagicByte = 0;
  #endif
}

/* ************************************************************************
   Function: GetLine
   Description:
     Returns pointer to specific line description.
*/
TLine *GetLine(const TFile *pFile, int nLine)
{
  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PARRAY(pFile->pIndex));
  ASSERT(nLine >= 0);
  ASSERT(nLine <= _TArrayCount(pFile->pIndex));

  if (nLine == _TArrayCount(pFile->pIndex))
    return NULL;  /* very last line */
  return &pFile->pIndex[nLine];
}

/* ************************************************************************
   Function: LineIsInBlock
   Description:
     Checks whether particular line falls in block.
*/
BOOLEAN LineIsInBlock(const TFile *pFile, int nLine)
{
  if (!pFile->bBlock)
    return (FALSE);

  if (nLine >= pFile->nStartLine && nLine <= pFile->nEndLine)
    return (TRUE);  /* This line is in the block */

  return (FALSE);
}

/* ************************************************************************
   Function: StoreFilePrim
   Description:
     Stores a file to the disk.
   On exit:
     0 -- the file has been successfully stored;
     1 -- failed to open the file for writing (errno);
     2 -- failed to store whole the file;
     3 -- no enough memory;
*/
int StoreFilePrim(const TFile *pFile, int nOutputEOLType)
{
  int nLine;
  int nPortionStart;
  char *pPortion;
  char *p;
  char *p2;
  int nPrevLineLen;
  int nEOLSize;
  int nOutputEOLSize;
  int i;
  TLine *pLine;
  int nToStore;
  int nStored;
  int nExitCode;
  FILE *f;
  int nSize;

  ASSERT(VALID_PFILE(pFile));

  nEOLSize = pFile->nEOLType == CRLFtype ? 2 : 1;
  nOutputEOLSize = nOutputEOLType == CRLFtype ? 2 : 1;
  nExitCode = 0;

  if (nEOLSize != nOutputEOLSize)
    goto _reformat_size;

  f = fopen(pFile->sFileName, WRITE_BINARY_FILE);
  if (f == NULL)
  {
    /*
    TODO: Display errno_msg
    */
    return 1;
  }
  nLine = 0;
  while (nLine < pFile->nNumberOfLines)
  {
    /*
    Mark the start of a portion.
    */
    pPortion = GetLineText(pFile, nLine);
    nPortionStart = nLine;

    /*
    Determine the end of the portion.
    */
    p = pPortion;
    while (1)
    {
      nPrevLineLen = GetLine(pFile, nLine)->nLen;
      ++nLine;
      if (nLine == pFile->nNumberOfLines) /* End Of File has been reached */
        break;
      p2 = GetLineText(pFile, nLine);
      if (p + nPrevLineLen + nEOLSize != p2)
        break;  /* end of the portion at nLine */
      p = p2;
    }

    /*
    Temporarily change the end-of-line marker
    (ASCIIZ string) from '\0' to the end-of-line
    as specified in nOutputEOLType
    */
    for (i = nPortionStart; i < nLine; ++i)
    {
      pLine = GetLine(pFile, i);
      p = pLine->pLine + pLine->nLen;  /* points at '\0' */
      switch (nOutputEOLType)
      {
        case CRLFtype:
          *p++ = '\r';
        case LFtype:
          *p++ = '\n';
          break;
        case CRtype:
          *p++ = '\r';
          break;
        default:
          ASSERT(0);  /* Invalid nOutputEOLType */
      }
    }  /* now _p_ points at the end of the portion */

    /*
    Store the portion to the disk.
    */
    nToStore = p - pPortion;
    nStored = fwrite(pPortion, 1, nToStore, f);
    if (nStored != nToStore)
      nExitCode = 2;  /* Failed to store whole the file */

    /*
    Return '\0' as end-of-line marker for the portion.
    */
    for (i = nPortionStart; i < nLine; ++i)
    {
      pLine = GetLine(pFile, i);
      p = pLine->pLine + pLine->nLen;
      *p = '\0';
    }
  }
  goto _close_file;

  /*
  As the size of the current pFile end-of-line marker differs
  from nOutputEOLType we'll need to allocate and transform all
  the lines from the file.
  Calculate the size of the file first, adding the new type of
  end-of-line marker.
  */
_reformat_size:
  nSize = 0;
  for (i = 0; i < pFile->nNumberOfLines; ++i)
    nSize += GetLine(pFile, i)->nLen + nOutputEOLSize;

  /*
  Allocate a block of memory where to transmorm the file
  */
  pPortion = xmalloc(nSize);
  if (pPortion == NULL)
    return 3;  /* No enough memory */

  p = pPortion;
  for (i = 0; i < pFile->nNumberOfLines; ++i)
  {
    pLine = GetLine(pFile, i);
    strcpy(p, pLine->pLine);
    p += pLine->nLen;  /* points at '\0' */
    switch (nOutputEOLType)
    {
      case CRLFtype:
        *p++ = '\r';
      case LFtype:
        *p++ = '\n';
        break;
      case CRtype:
        *p++ = '\r';
        break;
      default:
        ASSERT(0);  /* Invalid nOutputEOLType */
    }
    ASSERT(p - pPortion <= nSize);
  }
  ASSERT(p - pPortion == nSize);

  /*
  Store the transformed file to the disk.
  */
  f = fopen(pFile->sFileName, WRITE_BINARY_FILE);
  if (f == NULL)
  {
    /*
    TODO: Display errno_msg
    */
    xfree(pPortion);
    return 1;
  }
  nStored = fwrite(pPortion, 1, nSize, f);
  if (nStored != nSize)
    nExitCode = 2;  /* Failed to store whole the file */

  /*
  Dispose the temporary block as the file was stored.
  */
  xfree(pPortion);

_close_file:
  fclose(f);
  return nExitCode;
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

