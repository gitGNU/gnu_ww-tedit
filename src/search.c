/*

File: search.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 18th October, 1999
Descrition:
  Functions concerning text search and replace.

*/

#include "global.h"
#include "l1def.h"
#include "l2disp.h"
#include "memory.h"
#include "file.h"
#include "nav.h"
#include "undo.h"
#include "block.h"
#include "search.h"

#define STATIC
#include "pcre.h"  /* Perl regular expressions library */
#undef STATIC

/* ************************************************************************
   Function: InitSearchContext
   Description:
*/
void InitSearchContext(TSearchContext *pstSearchContext)
{
  ASSERT(pstSearchContext != NULL);

  memset(pstSearchContext, 0, sizeof(*pstSearchContext));

  #ifdef _DEBUG
  pstSearchContext->MagicByte = SEARCHCTX_MAGIC;
  #endif

  pstSearchContext->sSearch[0] = 0;
  pstSearchContext->nDirection = 1;
  pstSearchContext->bCaseSensitive = TRUE;

  pstSearchContext->bRegularExpr = FALSE;
  pstSearchContext->bRegAllocated = FALSE;

  pstSearchContext->psTextBuf = NULL;
  pstSearchContext->bHeap = FALSE;

  pstSearchContext->bSuccess = FALSE;
  pstSearchContext->nFileID = -1;
}

/* ************************************************************************
   Function: DisposeSubstrings
   Description:
     Called to dispose substrings that are allocated in the heap.
*/
static void DisposeSubstrings(TSearchContext *pstSearchContext)
{
  int i;

  for (i = 0; i < _countof(pstSearchContext->Substrings); ++i)
  {
    if (pstSearchContext->Substrings[i] == NULL)
      continue;

    if (pstSearchContext->Substrings[i] != pstSearchContext->SubstringBufs[i])
        s_free(pstSearchContext->Substrings[i]);
  }
  memset(pstSearchContext->Substrings, 0, sizeof(pstSearchContext->Substrings));
  memset(pstSearchContext->SubstringBufs, 0, sizeof(pstSearchContext->SubstringBufs));
}

/* ************************************************************************
   Function: DoneSearchContext
   Description:
     Disposes regular expression data, if any.
*/
void DoneSearchContext(TSearchContext *pstSearchContext)
{
  ASSERT(VALID_PSEARCHCTX(pstSearchContext));

  DisposeSubstrings(pstSearchContext);

  if (!pstSearchContext->bRegAllocated)
    return;  /* Nothing to dispose */

  pcre_free(pstSearchContext->pstRegExprData);
}

/* ************************************************************************
   Function: ClearSearchContext
   Description:
     Clears any elements from previous search.
*/
void ClearSearchContext(TSearchContext *pstSearchContext)
{
  DoneSearchContext(pstSearchContext);
  InitSearchContext(pstSearchContext);
}

/* ************************************************************************
   Function: NewSearch
   Description:
     New search is called whenever new parameters of a search
     are necessary to be established.
     The main reson for this function to be called is to dispose
     the last regular expression data if allocated from previous searches.
     Sets new search pattern. Compiles the pattern provided that it
     is a regular expression (pstSearchContext->bRegularExpr is TRUE)

   Returns:
     FALSE - regular expression error; The error message is in
*/
BOOLEAN NewSearch(TSearchContext *pstSearchContext, const char *sText,
  int nNumLines)
{
  int flags;
  const char *psError;

  ASSERT(VALID_PSEARCHCTX(pstSearchContext));
  ASSERT(strlen(sText) < sizeof(pstSearchContext->sSearch));

  pstSearchContext->bSuccess = FALSE;

  pstSearchContext->nNumLines = nNumLines;

  strcpy(pstSearchContext->sSearch, sText);

  if (pstSearchContext->bRegAllocated)
  {
    pcre_free(pstSearchContext->pstRegExprData);
    pstSearchContext->bRegAllocated = FALSE;
  }

  pstSearchContext->nErrorOffset = -1;
  pstSearchContext->sError[0] = '\0';
  if (!pstSearchContext->bRegularExpr)
    return TRUE;

  /*
  Prepare search context for a regular expression
  */
  flags = 0;
  if (!pstSearchContext->bCaseSensitive)
    flags |= PCRE_CASELESS;
  pstSearchContext->pstRegExprData = pcre_compile(pstSearchContext->sSearch,
    flags, &psError,
    &pstSearchContext->nErrorOffset, NULL);
  pstSearchContext->psRegExprError = (void *)psError;
  if (pstSearchContext->pstRegExprData == NULL)
    return FALSE;
  pstSearchContext->bRegAllocated = TRUE;
  return TRUE;
}

/* ************************************************************************
   Function: PrepareTextBuf
   Description:
     Prepares the textbuf in pstSearchContext.
     On exit pstSearchContext->psTextBuf points to the text
     where to search into.
     When multiple line search pattern is specified the correspondent number
     of lines are concatenated from the file in pstSearchContext->sTextBuf
     and pstSearchContext->psTextBuf points to this buffer. If the buffer
     is not enough to accomodate all the lines then a block in the heap
     is allocated (signaled by pstSearchContext->bHeap) and
     pstSearchContext->psTextBuf points to it.

   Important:
     DisposeTextBuf() should be called after the search operation is completed.

   Returns:
     FALSE - no memory for this operation.
*/
BOOLEAN PrepareTextBuf(TSearchContext *pstSearchContext,
  const TFile *pFile, int nSearchLine)
{
  int i;
  int nLen;
  char *d;
  TLine *pLine;

  ASSERT(VALID_PSEARCHCTX(pstSearchContext));
  ASSERT(pstSearchContext->bHeap == FALSE);

  if (pstSearchContext->nNumLines == 0 ||
    pstSearchContext->nNumLines + nSearchLine >= pFile->nNumberOfLines)
  {
    pstSearchContext->psTextBuf = GetLineText(pFile, nSearchLine);
    pstSearchContext->psTextBufLen = GetLine(pFile, nSearchLine)->nLen;
    return TRUE;
  }

  /*
  Do an estimation of the size of the buffer that
  can accomodate the lines.
  nNumLines means: get a line + \n + the next line
  */
  nLen = 0;
  for (i = 0; i <= pstSearchContext->nNumLines; ++i)
    nLen += GetLine(pFile, nSearchLine + i)->nLen + 1;

  /*
  Use either the static buffer or allocate memory from the heap
  */
  pstSearchContext->psTextBuf = pstSearchContext->sTextBuf;
  if (nLen + 1 > MAX_TEXTBUF)
  {
    pstSearchContext->psTextBuf = alloc(nLen + 1);
    pstSearchContext->bHeap = TRUE;
    if (pstSearchContext->psTextBuf == NULL)
      return FALSE;
  }

  /*
  Concat line in psTextBuf
  */
  d = pstSearchContext->psTextBuf;
  for (i = 0; i <= pstSearchContext->nNumLines; ++i)
  {
    pLine = GetLine(pFile, nSearchLine + i);
    strcpy(d, pLine->pLine);
    d += pLine->nLen;
    *d++ = '\n';
  }
  *--d = '\0';  /* remove the last \n (nNumLines accumulates enough lines) */
  return TRUE;
}

/* ************************************************************************
   Function: DisposeTextBuf
   Description:
     PrepareTextBuf() may need to allocate a block in the heap
     to concatenate when no enough room to concatenate lines, then
     DisposeTextBuf() disposes the so allocated buffer from the heap.
     No action if no buffer allocated.
*/
static void DisposeTextBuf(TSearchContext *pstSearchContext)
{
  ASSERT(VALID_PSEARCHCTX(pstSearchContext));

  if (pstSearchContext->bHeap)
    s_free(pstSearchContext->psTextBuf);
  pstSearchContext->bHeap = FALSE;
  pstSearchContext->psTextBuf = NULL;
  return;
}

/* ************************************************************************
   Function: RegularExprSearch
   Description:
     Same like Search(). Called from Search() whenever we have a regular
     expression compiled.
     Searches sSearch[], options: nDirection, bCaseSensitive.
   On exit:
     Returns TRUE and (*pSearchLn, *nSearchPos) hold a position with
     occurence.
     Returns FALSE when string not found.
*/
static BOOLEAN RegularExprSearch(TSearchContext *pstSearchContext, const TFile *pFile)
{
  char *pStr;
  int nFlags;
  int nStartLine;
  int nSearchPos;
  int nLine;
  int nPrevPos;
  int nStartRegion;
  int nEndRegion;
  int nCount;
  int Offsets[45];
  int i;
  int j;
  int nLen;
  char *p;
  char *p2;

  pstSearchContext->bPassedEndOfFile = FALSE;
  nStartLine = pstSearchContext->nSearchLine;
  nSearchPos = pstSearchContext->nSearchPos;
  nLine = nStartLine;

  DisposeSubstrings(pstSearchContext);

  /*
  Major 2 branches in this function are determined depending on the search direction. As the 
  regular expression library searches only forward we need to take special provisions to provide
  search-back functionality.
  */
  if (pstSearchContext->nDirection == 1)
  {
    while (1)
    {
      /*
      Prepare the search buffer
      */
      if (!PrepareTextBuf(pstSearchContext, pFile, nLine))
      {
        pstSearchContext->nError = 2;  /* no memory */
        return FALSE;
      }
      /* pStr points to the start position */
      pStr = pstSearchContext->psTextBuf;
      nFlags = 0;
      nCount = pcre_exec(pstSearchContext->pstRegExprData, NULL,
        pStr, strlen(pStr), nSearchPos, nFlags,
        Offsets, _countof(Offsets));
      if (nCount > 0)
      {
        /*
        Set the position of the match
        */
        pstSearchContext->nSearchLine = nLine;
        pstSearchContext->nSearchPos = Offsets[0];
        pstSearchContext->nEndLine = nLine + pstSearchContext->nNumLines;
        if (pstSearchContext->nNumLines == 0)
        {
          pstSearchContext->nEndPos = Offsets[1];
          if (Offsets[0] == Offsets[1])
            pstSearchContext->bPosOnly = TRUE;
        }
        else
        {
          /* look for how many characters we have before the last \n */
          p = pStr + Offsets[1];  /* last match pos */
          p2 = p;
          while (1)
          {
            if (*p == '\n')
            {
              pstSearchContext->nEndPos = p2 - p - 1;  /* -1 for \n */
              goto _exit_report;
            }
            --p;
            ASSERT(p >= pStr);
          }
        }
_exit_report:
        ASSERT(pstSearchContext->nSearchLine < pFile->nNumberOfLines);
        nLen = GetLine(pFile, pstSearchContext->nSearchLine)->nLen;
        ASSERT(pstSearchContext->nSearchPos <= nLen);
        ASSERT(pstSearchContext->nEndLine >= nLine);
        ASSERT(pstSearchContext->nEndLine < pFile->nNumberOfLines);
        ASSERT(pstSearchContext->nEndPos <=
          GetLine(pFile, pstSearchContext->nEndLine)->nLen);

        /*
        Extract the substrings
        */
        for (i = 0, j = 0; i < nCount; ++i, ++j)
        {
          if (Offsets[i * 2] < 0)
            continue;  /* the substring is unset */
          nLen = Offsets[i * 2 + 1] - Offsets[i * 2];
          pstSearchContext->Substrings[j] = pstSearchContext->SubstringBufs[j];
          if (nLen + 1 > MAX_SUBPATTERN_LEN)
          {
            pstSearchContext->Substrings[j] = alloc(nLen + 1);
            if (pstSearchContext->SubstringBufs[j] == NULL)
            {
              strcpy(pstSearchContext->sError, "no enough memory");
              pstSearchContext->nErrorOffset = -1;
              return FALSE;
            }
            memset(pstSearchContext->Substrings[j], 0, nLen + 1);
          }
          strncpy(pstSearchContext->Substrings[j], pStr + Offsets[i * 2], nLen);
        }

        /* Indicate success */
        return TRUE;
      }
      /*
      No match at this line -- advance to the next
      */
      ++nLine;
      nSearchPos = 0;
      if (nLine == pFile->nNumberOfLines)
      {
        nLine = 0;
        pstSearchContext->bPassedEndOfFile = TRUE;
      }
      if (nLine == nStartLine)
        return FALSE;    /* We've reached the position we started from, no occurrence */
    }
  }
  else
  {
    /*
    Search backward.
    */
    ASSERT(pstSearchContext->nDirection == -1);
    while (1)
    {
      /*
      Prepare the search buffer
      */
      if (!PrepareTextBuf(pstSearchContext, pFile, nLine))
      {
        pstSearchContext->nError = 2;  /* no memory */
        return FALSE;
      }
      pStr = pstSearchContext->psTextBuf;
      nStartRegion = 0;
      if (nSearchPos != -1)
        nEndRegion = nSearchPos;
      else
        nEndRegion = strlen(pStr);
      nPrevPos = -1;  /* No occurence */

      /*
      Search in the current line (nLine)
      */
_retry_line:
      nFlags = 0;
      nCount = pcre_exec(pstSearchContext->pstRegExprData, NULL,
        pStr, nEndRegion - nStartRegion, nStartRegion, nFlags,
        Offsets, _countof(Offsets));
      if (nCount > 0)
      {
        /*
        Set the position of the match
        */
        nPrevPos = Offsets[0];
        if (pstSearchContext->nNumLines == 0)
          pstSearchContext->nEndPos = Offsets[1];
        else
        {
          /* look for how many characters we have before the last \n */
          p = pStr + Offsets[1];  /* last match pos */
          p2 = p;
          while (1)
          {
            if (*p == '\n')
            {
              pstSearchContext->nEndPos = p2 - p - 1;  /* -1 for \n */
              goto _extract_subs;
            }
            --p;
            ASSERT(p > pStr);
          }
        }

        /*
        Extract the substrings
        */
_extract_subs:
        for (i = 0, j = 0; i < nCount; ++i, ++j)
        {
          if (Offsets[i * 2] < 0)
            continue;  /* the substring is unset */
          nLen = Offsets[i * 2 + 1] - Offsets[i * 2];
          pstSearchContext->Substrings[j] = pstSearchContext->SubstringBufs[j];
          if (nLen + 1 > MAX_SUBPATTERN_LEN)
          {
            pstSearchContext->Substrings[j] = alloc(nLen + 1);
            if (pstSearchContext->SubstringBufs[j] == NULL)
            {
              strcpy(pstSearchContext->sError, "no enough memory");
              pstSearchContext->nErrorOffset = -1;
              return FALSE;
            }
            memset(pstSearchContext->Substrings[j], 0, nLen + 1);
          }
          strncpy(pstSearchContext->Substrings[j], pStr + Offsets[i * 2], nLen);
        }

        nStartRegion = Offsets[1];
        if (Offsets[0] == Offsets[1])
          pstSearchContext->bPosOnly = TRUE;  /* and end of loop */
        else
          goto _retry_line;
      }
      /*
      We have no occurence in the specified region
      Check for previous occurence and return the result
      */
      if (nPrevPos != -1)
      {
        /* We already have something */
        /* Set the position of the match */
        pstSearchContext->nSearchLine = nLine;
        pstSearchContext->nSearchPos = nPrevPos;
        pstSearchContext->nEndLine = nLine + pstSearchContext->nNumLines;

        ASSERT(pstSearchContext->nSearchLine < pFile->nNumberOfLines);
        ASSERT(pstSearchContext->nSearchPos <= GetLine(pFile, pstSearchContext->nSearchLine)->nLen);
        ASSERT(pstSearchContext->nEndLine >= nLine);
        ASSERT(pstSearchContext->nEndLine < pFile->nNumberOfLines);
        ASSERT(pstSearchContext->nEndPos <=
          GetLine(pFile, pstSearchContext->nEndLine)->nLen);
        return TRUE;
      }
      /* Move to prevous line */
      --nLine;
      nSearchPos = -1;  /* This is valid only at the first line */
      if (nLine == nStartLine)
        return FALSE;    /* We've reached the position we started from, no occurrence */
      if (nLine == -1)
      {
        pstSearchContext->bPassedEndOfFile = TRUE;
        nLine = pFile->nNumberOfLines - 1;
        ASSERT(nLine >= 0);
      }
    }
  }
  return FALSE;
}

/* ************************************************************************
   Function: Search
   Description:
     Searches sSearch[], options: nDirection, bCaseSensitive, bRegularExpr.
   On exit:
     Returns TRUE and (*pSearchLn, *nSearchPos) hold a position with
     occurence.
     Returns FALSE when string not found.
     Check pstSerchContext->nError:
     1 - regular expression error, pstSearchContext->sError for text
     2 - no memory for the operation;
*/
static BOOLEAN Search(TSearchContext *pstSearchContext, const TFile *pFile)
{
  char *pStr;
  char *p;
  char *d;
  char *p2;
  char *d2;
  char LocalStr[MAX_SEARCH_STR];
  int nPatternLen;
  int nTextLen;
  int nStartLine;
  int nSearchLine;
  int nSearchPos;
  int x;
  BOOLEAN bResult;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PSEARCHCTX(pstSearchContext));

  ASSERT(pstSearchContext->sSearch[0] != '\0');

  if (pstSearchContext->nNumLines > pFile->nNumberOfLines)
    return FALSE;

  nSearchLine = pstSearchContext->nSearchLine;
  nSearchPos = pstSearchContext->nSearchPos;
  pstSearchContext->bPassedEndOfFile = FALSE;
  ASSERT(nSearchPos <= GetLine(pFile, nSearchLine)->nLen);

  pstSearchContext->nError = 0;
  if (pstSearchContext->bRegularExpr)
  {
    bResult = RegularExprSearch(pstSearchContext, pFile);
    DisposeTextBuf(pstSearchContext);
    if (bResult)
      goto _mark_success;
    return FALSE;
  }

  pstSearchContext->bPassedEndOfFile = FALSE;

  strcpy(LocalStr, pstSearchContext->sSearch);
  if (!pstSearchContext->bCaseSensitive)
    strlwr(LocalStr);
  nPatternLen = strlen(LocalStr);

  /*
  Prepare the search buffer
  */
  if (!PrepareTextBuf(pstSearchContext, pFile, nSearchLine))
  {
    pstSearchContext->nError = 2;  /* no memory */
    return FALSE;
  }
  if (pstSearchContext->psTextBuf == pstSearchContext->sTextBuf)
    ASSERT(GetLine(pFile, nSearchLine)->nLen == pstSearchContext->psTextBufLen);

  nStartLine = nSearchLine;

  /*
  Two major branches of this function depending on
  the search direction -- forward (nDirection is 1) or backward (nDirection is -1)
  */
  if (pstSearchContext->nDirection == 1)
  {
    /* pStr is where we start to search from */
    pStr = pstSearchContext->psTextBuf + nSearchPos;

    if (nSearchLine == pFile->nNumberOfLines)
      goto _advance_line;

    d = LocalStr;
    nTextLen = strlen(pStr);
    if (nTextLen < nPatternLen)
      goto _advance_line;  /* no enough text to match */

    while (1)
    {
      p = pStr;
      /* check that p is pointing withing psTextBuf */
      ASSERT(p >= pstSearchContext->psTextBuf &&
        p <= pstSearchContext->psTextBuf + pstSearchContext->psTextBufLen);

      while (1)
      {
        if (*p == '\0')
          goto _advance_line;
        /* short-cut: we have extracted precise number of lines */
        if (*p == '\n')  /* if we pass the first without start of a pattern */
          goto _advance_line;  /* then failure is guaranteed */
        if (pstSearchContext->bCaseSensitive)
        {
          if (*p == *d)
            goto _compare_all;
        }
        else
          if (tolower(*p) == *d)
            goto _compare_all;
_continue_search:
        ++p;
      }

_advance_line:
      /*
      Search forward:
      Advance to the next line.
      */
      ++nSearchLine;
      nSearchPos = 0;
      if (nSearchLine == nStartLine)
        return FALSE;  /* We've reached the position we started from, no occurrence */
      if (nSearchLine + pstSearchContext->nNumLines >= pFile->nNumberOfLines)
      {
        if (pstSearchContext->bPassedEndOfFile)
          return FALSE;
        pstSearchContext->bPassedEndOfFile = TRUE;
        nSearchLine = 0;
      }
      /* We need new lines text to be searched for the pattern */
      if (!PrepareTextBuf(pstSearchContext, pFile, nSearchLine))
      {
        pstSearchContext->nError = 2;  /* no memory */
        return FALSE;
      }

      pStr = pstSearchContext->psTextBuf;
      continue;

_compare_all:
      /*
      Search forward:
      Compare the rest of the string.
      */
      if (LocalStr[1] == '\0')  /* Search string is only 1 character long */
      {
_report_success:
        /* Search forward: */
        pstSearchContext->nSearchLine = nSearchLine;
        /* check that p is pointing withing psTextBuf */
        ASSERT(p >= pstSearchContext->psTextBuf &&
          p <= pstSearchContext->psTextBuf + pstSearchContext->psTextBufLen);
        nSearchPos = p - pstSearchContext->psTextBuf;
        ASSERT(nSearchPos <= GetLine(pFile, nSearchLine)->nLen);
        pstSearchContext->nSearchPos = nSearchPos;
        pstSearchContext->nEndLine = nSearchLine + pstSearchContext->nNumLines;
        if (pstSearchContext->nNumLines == 0)
          pstSearchContext->nEndPos = nSearchPos + strlen(LocalStr);
        else
        {
          /* look for how many characters we have before the last \n */
          p = strchr(pstSearchContext->sSearch, '\0');
          p2 = p;
          while (p > pstSearchContext->sSearch)
          {
            if (*p == '\n')
            {
              pstSearchContext->nEndPos = p2 - p - 1;  /* -1 for \n */
              goto _exit_report;
            }
            --p;
          }
          ASSERT(p != pstSearchContext->sSearch);  /* no \n and nNumLine>0 */
        }
_exit_report:
        /* Search forward: */
        ASSERT(nSearchLine < pFile->nNumberOfLines);
        ASSERT(nSearchPos < GetLine(pFile, nSearchLine)->nLen);
        ASSERT(nSearchPos >= 0);
        ASSERT(pstSearchContext->nEndLine >= nSearchLine);
        ASSERT(pstSearchContext->nEndLine < pFile->nNumberOfLines);
        ASSERT(pstSearchContext->nEndPos <=
          GetLine(pFile, pstSearchContext->nEndLine)->nLen);
        if (pstSearchContext->nEndPos < 0)
          ASSERT(pstSearchContext->nNumLines == 0);
_mark_success:
        pstSearchContext->bSuccess = TRUE;
        return TRUE;
      }

      p2 = p;
      d2 = d;
      while (1)
      {
        if (pstSearchContext->bCaseSensitive)
        {
          if (*p2 == *d2)
            goto _advance;
        }
        else
        {
          if (tolower(*p2) == *d2)
            goto _advance;
        }
        goto _continue_search;

_advance:
        d2++;
        if (*d2 == '\0')
        {
          nSearchPos = p - pstSearchContext->psTextBuf;
          goto _report_success;
        }
        p2++;
        if (*p2 == '\0')
          goto _continue_search;
      }
    }
    return FALSE;
  }
  else
  {
    ASSERT(pstSearchContext->nDirection == -1);

    /* pStr is where we start to search from */
    pStr = pstSearchContext->psTextBuf + nSearchPos;

    nStartLine = nSearchLine;

    if (nSearchLine == pFile->nNumberOfLines)
      goto _advance_line2;

    d = strchr(LocalStr, '\0') - 1;
    nTextLen = strlen(pstSearchContext->psTextBuf);
    if (nTextLen < nPatternLen)
      goto _advance_line2;  /* No enough simbols at this line */
    x = nTextLen - nPatternLen;
    /* pStr at the first pos with enough simbols before or at nSearchPos */
    pStr = pstSearchContext->psTextBuf + min(x, nSearchPos);

    while (1)
    {
      p = pStr;

      p += nPatternLen - 1;

      while (1)
      {
        if (pstSearchContext->bCaseSensitive)
        {
          if (*p == *d)
            goto _compare_all2;
        }
        else
          if (tolower(*p) == *d)
            goto _compare_all2;
_continue_search2:
        if (p == pstSearchContext->psTextBuf)
          break;  /* we got to the start of the line */
        --p;
      }

_advance_line2:
      /*
      Search backward:
      Advance to the previous line.
      */
      --nSearchLine;
      nSearchPos = 0;
      if (nSearchLine == nStartLine)
        return FALSE;
      if (nSearchLine == -1)
      {
        pstSearchContext->bPassedEndOfFile = TRUE;
        nSearchLine = pFile->nNumberOfLines - 1;
        ASSERT(nSearchLine >= 0);
      }
      /* Ensure enough lines for PrepareTextBuf() to operate */
      if (nSearchLine + pstSearchContext->nNumLines >= pFile->nNumberOfLines)
        nSearchLine = pFile->nNumberOfLines - pstSearchContext->nNumLines;
      ASSERT(nSearchLine >= 0);
      /* We need new lines text to be searched for the pattern */
      if (!PrepareTextBuf(pstSearchContext, pFile, nSearchLine))
      {
        pstSearchContext->nError = 2;  /* no memory */
        return FALSE;
      }

      /* the case when there are no enough symbols in the buffer */
      nTextLen = strlen(pstSearchContext->psTextBuf);
      if (nTextLen < nPatternLen)
        goto _advance_line2;  /* no enough text to match */

      pStr = strchr(pstSearchContext->psTextBuf, '\0');
      ASSERT(pStr != NULL);
      pStr -= nPatternLen;  /* first pos with enough symbols */
      continue;

_compare_all2:
      /*
      Search backward:
      Compare the rest of the string.
      */
      if (LocalStr[1] == '\0')  /* Search string is only 1 character long */
      {
        nSearchPos = p - pstSearchContext->psTextBuf;
_report_success2:
        /* Search backward: */
        pstSearchContext->nSearchLine = nSearchLine;
        pstSearchContext->nSearchPos = nSearchPos;
        pstSearchContext->nEndLine = nSearchLine + pstSearchContext->nNumLines;
        if (pstSearchContext->nNumLines == 0)
          pstSearchContext->nEndPos = nSearchPos + strlen(LocalStr);
        else
        {
          /* look for how many characters we have before the last \n */
          p = strchr(pstSearchContext->sSearch, '\0');
          p2 = p;
          while (p > pstSearchContext->sSearch)
          {
            if (*p == '\n')
            {
              pstSearchContext->nEndPos = p2 - p - 1;  /* -1 for \n */
              goto _exit_report2;
            }
            --p;
          }
          ASSERT(p != pstSearchContext->sSearch);  /* no \n and nNumLine>0 */
        }
_exit_report2:
        ASSERT(nSearchLine < pFile->nNumberOfLines);
        ASSERT(nSearchPos < GetLine(pFile, nSearchLine)->nLen);
        ASSERT(pstSearchContext->nEndLine >= nSearchLine);
        ASSERT(pstSearchContext->nEndLine < pFile->nNumberOfLines);
        ASSERT(pstSearchContext->nEndPos <=
          GetLine(pFile, pstSearchContext->nEndLine)->nLen);
        goto _mark_success;
      }

      p2 = p;
      d2 = d;
      while (1)
      {
        if (pstSearchContext->bCaseSensitive)
        {
          if (*p2 == *d2)
            goto _advance2;
        }
        else
        {
          if (tolower(*p2) == *d2)
            goto _advance2;
        }
        goto _continue_search2;

_advance2:
        if (d2 == LocalStr)
        {
          nSearchPos = p2 - pstSearchContext->psTextBuf;
          goto _report_success2;
        }
        --d2;
        if (p2 == pstSearchContext->psTextBuf)
          goto _continue_search2;
        --p2;
      }
    }
    return FALSE;
  }
}

/* ************************************************************************
   Function: PutISearchMsg
   Description:
     Prepares a message to be put on the status line
     indicating that an incremental search is in progress.
*/
static void PutISearchMsg(TFile *pFile, int nWidth, 
  const TSearchContext *pstSearchContext, BOOLEAN bPassedEOF)
{
  char sBuf[MAX_FILE_MSG_LEN];
  int nLen;
  int nBufLen;
  int nMsgEOFLen;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(nWidth < MAX_FILE_MSG_LEN);

  memset(sBuf, ' ', sizeof(sBuf));

  if (pstSearchContext->nDirection == 1)
    strcpy(sBuf, sIncrementalSearch);
  else
    strcpy(sBuf, sIncrementalSearchBack);

  nLen = strlen(pstSearchContext->sSearch);
  nBufLen = strlen(sBuf);

  if (nLen + nBufLen > nWidth)
    strcat(sBuf, pstSearchContext->sSearch + nWidth - nBufLen - 2);
  else
    strcat(sBuf, pstSearchContext->sSearch);

  if (bPassedEOF)
  {
    nBufLen = strlen(sBuf);
    nMsgEOFLen = strlen(sPassedEndOfFile);
    if (nBufLen + nMsgEOFLen > nWidth)
      strcpy(&sBuf[nWidth - nMsgEOFLen - 1], sPassedEndOfFile);
    else
    {
      sBuf[nBufLen] = ' ';
      strcpy(&sBuf[nBufLen + (nWidth - nMsgEOFLen - nBufLen)], sPassedEndOfFile);
    }
  }

  strcpy(pFile->sMsg, sBuf);
  pFile->bUpdateStatus = TRUE;
}

/* ************************************************************************
   Function: SearchOccurence
   Description:
     Searches next occurence of sSearch in a file. Called by Ctrl+I/F3 command
     Positions the cursor at the occurence.
     Marks the occurence.
*/
static BOOLEAN SearchOccurence(TFile *pFile,
  TSearchContext *pstSearchContext, BOOLEAN bAllowPassingEOF, BOOLEAN *pbPassEOF)
{
  ASSERT(VALID_PFILE(pFile));

  *pbPassEOF = FALSE;

  if (pFile->nNumberOfLines == 0)
    return FALSE;

  /*
  We can not do search if the cursor is beyond the end of the file.
  */
  if (pFile->nRow == pFile->nNumberOfLines)
  {
    if (pstSearchContext->nDirection == 1)
    {
      *pbPassEOF = TRUE;
      pstSearchContext->bPassedEndOfFile = TRUE;
      if (!bAllowPassingEOF)
        return FALSE;
      GotoColRow(pFile, 0, 0);
    }
    else
    {
      ASSERT(pstSearchContext->nDirection == -1);
      LineUp(pFile);
      GotoEndPosition(pFile);
    }
  }

  pstSearchContext->nSearchLine = pFile->nRow;  /* start from here */
  pstSearchContext->nSearchPos = INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos);
  pstSearchContext->bSuccess = FALSE;
  if (!Search(pstSearchContext, pFile))
    return FALSE;
  pstSearchContext->bSuccess = TRUE;
  *pbPassEOF = pstSearchContext->bPassedEndOfFile;
  if (*pbPassEOF)
  {
    if (!bAllowPassingEOF)
      return FALSE;
  }

  /*
  Do a selection over the text of the pattern
  */
  ASSERT(pstSearchContext->nSearchPos >= 0);
  ASSERT(pstSearchContext->nEndPos >= -1);
  GotoPosRow(pFile, pstSearchContext->nEndPos, pstSearchContext->nEndLine);
  MarkBlockEnd(pFile);
  GotoPosRow(pFile, pstSearchContext->nSearchPos, pstSearchContext->nSearchLine);
  MarkBlockBegin(pFile);
  pFile->bPreserveSelection = TRUE;
  return TRUE;
}

/* ************************************************************************
   Function: IncrementalSearch
   Description:
     General Incremental Search activation and support routine.
     Aaccumulates the search string and invokes search routine
     to find next occurence.
*/
void IncrementalSearch(TFile *pFile, char *c, int nWidth,
  TSearchContext *pstSearchContext)
{
  char *p;
  BOOLEAN bPassedEOF;

  p = strchr(pstSearchContext->sSearch, '\0');
  strcat(pstSearchContext->sSearch, c);
  pstSearchContext->nFileID = pFile->nID;
  if (!SearchOccurence(pFile, pstSearchContext, TRUE, &bPassedEOF))
  {
    *p = '\0';  /* Cut the last character as no match found */
    bPassedEOF = FALSE;
  }
  PutISearchMsg(pFile, nWidth, pstSearchContext, bPassedEOF);
}

/* ************************************************************************
   Function: IncrementalSearchRemoveLast
   Description:
     Removes the last character of the serach pattern.
     Activates the last occurence of the changed search pattern.
*/
void IncrementalSearchRemoveLast(TFile *pFile, int nWidth,
  TSearchContext *pstSearchContext)
{
  char *p;
  BOOLEAN bPassedEOF;
  BOOLEAN bDummy;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PSEARCHCTX(pstSearchContext));
  p = strchr(pstSearchContext->sSearch, '\0');
  if (p != pstSearchContext->sSearch)
    *(p - 1) = '\0';
  PutISearchMsg(pFile, nWidth, pstSearchContext, FALSE);
  if (pFile->bBlock)
    ToggleBlockHide(pFile);
  /* Start searching from the pre-i-search position
  this way, go to the first occurence (original cursor relevant) in 
  the file of the new string */
  bPassedEOF = FALSE;
  if (nPreISearch_Row > pFile->nRow)
    bPassedEOF = TRUE;
  GotoColRow(pFile, nPreISearch_Col, nPreISearch_Row);
  if (pstSearchContext->sSearch[0] != 0)
    SearchOccurence(pFile, pstSearchContext, TRUE, &bDummy);
  PutISearchMsg(pFile, nWidth, pstSearchContext, bPassedEOF);
}

/* ************************************************************************
   Function: Find
   Description:
     Called on response of F3-ShiftF3,
     or to find next occurence of the last search pattern.
   Returns:
     Cursor goes to the new position of a match and the match is
     selected.
*/
BOOLEAN Find(TFile *pFile, int nDir, int nWidth, TSearchContext *pstSearchContext)
{
  int nSaveDir;
  int nSaveCol;
  int nSaveRow;
  BOOLEAN bAllowPassingEOF;
  BOOLEAN bPassedEOF;
  int nLineLen;
  int nCurPos;
  BOOLEAN bFirstTime;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PSEARCHCTX(pstSearchContext));
  ASSERT(nDir == 1 || nDir == -1);

  /* Check if pstSearchContext represents a previous search operation
  into the same file */
  if (pstSearchContext->nFileID != pFile->nID)
  {
    pstSearchContext->nFileID = pFile->nID;
    pstSearchContext->bSuccess = FALSE;
    pstSearchContext->nDirection = nDir;
  }

  nSaveDir = pstSearchContext->nDirection;
  pstSearchContext->nDirection = nDir;  /* Temporarily change the direction */

  bAllowPassingEOF = TRUE;
  if (!bIncrementalSearch)
  {
    bAllowPassingEOF = FALSE;
    if (pstSearchContext->sSearch[0] == '\0')
      return FALSE;  /* TODO: MUST call input find string */
  }

  nSaveCol = pFile->nCol;
  nSaveRow = pFile->nRow;

  bFirstTime = FALSE;  /* assume this pattern is searched for the first time */
  if (pstSearchContext->bSuccess)
  {
    /*
    To start the search move the cursor one character in the desired direction
    */
    if (pstSearchContext->bPosOnly)
    {
      /* Our last occurence was positional only, no selection */
      if (nDir == -1)
      {
        GotoColRow(pFile, 0, nSaveRow -1);
        GotoEndPosition(pFile);
      }
      else
      {
        if (pFile->nRow >= pFile->nNumberOfLines - 1)
        {
          pstSearchContext->bSuccess = FALSE;
          goto _after_search;  /* nowhere further to advance */
        }
        nLineLen = GetLine(pFile, nSaveRow)->nLen;
        nCurPos = INDEX_IN_LINE(pFile, nSaveRow, pFile->pCurPos);
        if (nCurPos == nLineLen)
          GotoColRow(pFile, 0, nSaveRow + 1);
        else
          GotoColRow(pFile, nSaveCol + 1, nSaveRow);
      }
      if (pFile->nCol == nSaveCol
          && pFile->nRow == nSaveRow)  /* pos unchanged? */
      {
        pstSearchContext->bSuccess = FALSE;
        goto _after_search;  /* nowhere further to advance */
      }
    }
    else
    {
      if (pFile->bBlock  /* Cursor is still on top of the selection? */
        && pFile->nRow == pstSearchContext->nSearchLine
        && INDEX_IN_LINE(pFile, pFile->nRow, pFile->pCurPos) == pstSearchContext->nSearchPos)
      {
        if (pstSearchContext->bRegularExpr)
        {
          /* Leap accross the selection in case of reg-expr search */
          if (pstSearchContext->nDirection == 1)
            LeapThroughSelection(pFile, TRUE);
          else
            LeapThroughSelection(pFile, FALSE);
        }
        else
        {
          if (nSaveCol + nDir < 0)
          {
            GotoColRow(pFile, 0, nSaveRow - 1);
            GotoEndPosition(pFile);
          }
          else
            GotoColRow(pFile, nSaveCol + nDir, nSaveRow);
        }
      }
    }
  }
  else  /* no trace of successful search */
    bFirstTime = TRUE;  /* F3 on a pattern or search/replace loop */

  if (pFile->nRow < pFile->nNumberOfLines)  /* Not on ---no more messages--- ? */
    ASSERT(IS_VALID_CUR_POS(pFile));

  if (!SearchOccurence(pFile, pstSearchContext, bAllowPassingEOF, &bPassedEOF))
  {
    /* No new occurence, just keep the mark over the last */
    GotoColRow(pFile, nSaveCol, nSaveRow);
    pstSearchContext->bSuccess = FALSE;
    if (pFile->bBlock)
    {
      pFile->bBlock = FALSE;
      pFile->bUpdatePage = TRUE;
    }
  }

_after_search:
  if (bIncrementalSearch)
  {
    bPreserveIncrementalSearch = TRUE;
    PutISearchMsg(pFile, nWidth, pstSearchContext, bPassedEOF);
  }
  else
    if (nSaveCol != pFile->nCol || nSaveRow != pFile->nRow)
      pFile->bPreserveSelection = TRUE;  /* Preserve selection of the occurrence */
    else
      pFile->bPreserveSelection = FALSE;  /* Fag is not necessary in this case */

  pstSearchContext->nDirection = nSaveDir;

  if (bFirstTime)
  {
    pstSearchContext->nRowOfFirstOccurrence = pFile->nRow;
    pstSearchContext->nColOfFirstOccurrence = pFile->nCol;
  }

  return pstSearchContext->bSuccess;
}

/* ************************************************************************
   Function: Replace
   Description:
     This function is called when the cursor as positioned at an occurence.
     It deletes the selected text, prepares the replace string (this is
     necessary in case replace patterns are used), and pastes the replaced
     string.
*/
BOOLEAN Replace(TSearchContext *pstSearchCtx, TFile *pFile)
{
  int nUndoEntry;
  BOOLEAN bResult;
  int i;
  int nLen;
  char sBuf[1024];  /* This buffer will be enough in most of the cases */
  char *psRepl;
  TBlock *pReplBlock;

  /*
  Compose the block of text of the replace text
  (it may be compounded multi-pattern and multi-line string)

  1. Calculate the size of the string buffer and allocate a buffer.
  */
  nLen = 0;
  for (i = 0; i < pstSearchCtx->nNumRepl; ++i)
    if (pstSearchCtx->Repl[i].type == 0)
      nLen += strlen(pstSearchCtx->Repl[i].data.sReplString);
    else
      nLen += strlen(pstSearchCtx->Substrings[pstSearchCtx->Repl[i].data.pattern]);

  if (nLen + 1 < sizeof(sBuf))
    psRepl = sBuf;
  else
  {
    /* The buffer is not big enough to accomodate the new string */
    psRepl = alloc(nLen);
    if (psRepl == NULL)
      return FALSE;
  }

  /*
  2. Concat all the strings and pattern extractions
  */
  *psRepl = '\0';
  for (i = 0; i < pstSearchCtx->nNumRepl; ++i)
    if (pstSearchCtx->Repl[i].type == 0)
      strcat(psRepl, pstSearchCtx->Repl[i].data.sReplString);
    else
      strcat(psRepl, pstSearchCtx->Substrings[pstSearchCtx->Repl[i].data.pattern]);

  /*
  3. Make a TBlock of the string
  */
  pReplBlock = MakeBlock(psRepl, 0);
  if (psRepl != sBuf)
    s_free(psRepl);  /* We use a buffer in the heap, not the local sBuf */
  if (pReplBlock == NULL)
    return FALSE;

  /*
  The replace operation is compounded of two suboperations
  1. Remove the selected occurence
  2. Paste the replacement text

  UNDO_BLOCK_BEGIN/END() brackets the compounded operation
  to be regarded as a single operation by the Undo/Redo engine.
  */
  nUndoEntry = UNDO_BLOCK_BEGIN();

  if (!pstSearchCtx->bPosOnly)  /* Something to remove or just a pos? */
  {
    bResult = DeleteBlock(pFile);
    if (!bResult)
      goto _exit_point;
  }

  bResult = Paste(pFile, pReplBlock);
  if (!bResult)
    goto _exit_point;

_exit_point:
  DisposeABlock(&pReplBlock);
  UNDO_BLOCK_END(nUndoEntry, bResult);
  return bResult;
}

/* ************************************************************************
   Function: _CmdIncrementalSearch
   Description:
     Called from CmdIncrementalSearch() from CmdIncrementalSearchBack().
     Determines whether this is first call or a consequtive call by
     examining the global flag bIncrementalSearch (l1opt.c)
*/
void _CmdIncrementalSearch(TFile *pFile, int nDir, int nWidth, 
  TSearchContext *pstSearchContext)
{
  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PSEARCHCTX(pstSearchContext));
  ASSERT(nDir == 1 || nDir == -1);

  if (bIncrementalSearch)
  {
    if (pstSearchContext->sSearch[0] != '\0')
    {
      Find(pFile, nDir, nWidth, pstSearchContext);
      return;
    }
  }

  ClearSearchContext(pstSearchContext);
  pstSearchContext->bRegularExpr = FALSE;
  NewSearch(pstSearchContext, "", 0);
  pstSearchContext->nDirection = nDir;
  pstSearchContext->bCaseSensitive = FALSE;
  bIncrementalSearch = TRUE;
  bPreserveIncrementalSearch = TRUE;
  PutISearchMsg(pFile, nWidth, pstSearchContext, FALSE);

  /*
  Preserve current cursor positions to be restored on <Esc>
  */
  nPreISearch_Col = pFile->nCol;
  nPreISearch_Row = pFile->nRow;
}

/* ************************************************************************
   Function: ExtractComponent
   Description:
     Extracts a component
     Operates on quoted strings
     Valid elements:
         text\"text (you may escape the quote in a non-quoted string)
         "text" (quoted text)
         "text text1" (quoted string that has spaces)
         "text \"text2\"" (quoted string that has spaces and quotes, this is to 
         search for: text "text2" pattern
     Returns:
         New position
*/
const char *ExtractComponent(const char *psPos, char *psDest, BOOLEAN *pbQuoted)
{
  const char *p;
  char *c;
  BOOLEAN bNeedsQuote;

  p = psPos;
  if (*p == '\0')
    return NULL;

  memset(psDest, '\0', _MAX_PATH);

  /* skip leading blanks */
  while (*p == ' ')
    ++p;
  /* Extract the pattern; Check for quoted string
  or for escaped quote character */
  c = psDest;
  *c = '\0';
  bNeedsQuote = FALSE;
  if (*p == '\"')
  {
    bNeedsQuote = TRUE;
    ++p;
  }
  while (*p != '\0')
  {
    if (*p == ' ')
      if (!bNeedsQuote)
        break;
    if (*p == '\"')
      if (bNeedsQuote)
      {
        ++p;
        break;
      }
    if (*p == '\\')
    {
      if (*(p + 1) == '\"')
      {
        *c++ = '\"';
        p += 2;
        continue;
      }
    }
    *c++ = *p++;
  }
  *c = '\0';
  *pbQuoted = bNeedsQuote;
  if (*p == '\0')
    return NULL;
  return p;
}

/* ************************************************************************
   Function: ParseSearchPattern
   Description:
     Parses search/replace pattern. Result in pCtx.
   Returns:
     error code
     0 - ok
     1 - invalid option
     2 - invalid termination of replace pattern
     3 - invalid termination of search pattern
     4 - invalid pattern number
     5 - patterns are not allowed if no regular expression mode is specified
     6 - invalid escape - "\" at the end of the string
     7 - invalid reg expr pattern
   Pattern format:
     [m|s]/search_pattern/replace_pattern/[g]
*/
int ParseSearchPattern(const char *sPattern, TSearchContext *pCtx)
{
  char const *p;
  char const *save_p;
  char *d;
  char sSearch[MAX_SEARCH_STR];
  char sReplace[MAX_SEARCH_STR];
  int nRepl;
  BOOLEAN bReplace;
  BOOLEAN bExpectExtended;
  BOOLEAN bExplicitMatch;
  int nNumLines;
  BOOLEAN bStartReplPattern;  /* Indicates there is repl pattern */

  p = sPattern;
  sSearch[0] = '\0';
  sReplace[0] = '\0';
  bReplace = FALSE;
  bExplicitMatch = FALSE;  /* assume no m/s at the start */
  bStartReplPattern = FALSE;  /* assume no replace pattern specified */
  pCtx->nNumRepl = 0;
  nNumLines = 0;
  nRepl = 0;
  bExpectExtended = FALSE;
  switch (*p)
  {
    case '\0':
      NewSearch(pCtx, sSearch, 0);
      return 0;

    case 's':
      bReplace = TRUE;
    case 'm':
      if (*(p + 1) != '/')
        /* The whole text is only a search pattern */
        break;
      bExplicitMatch =  *p == 'm';
      p += 2;
_extended:
      bExpectExtended = TRUE;  /* look for the '/' end marker */
      pCtx->bRegularExpr = TRUE;
      break;

    case '/':
      ++p;
      goto _extended;

    default:
      /* The whole text is only a search pattern */
      break;
  }

  /*
  Extract the search pattern
  */
  d = sSearch;
  while (*p)
  {
    if (*p == '\\')
    {
      if (*(p + 1) == 'n')
      {
        ++nNumLines;
        p += 2;  /* skip over '\n' */
        *d++ = '\n';
        continue;
      }
      else
        if (*(p + 1) == '\\' || *(p + 1) == '/')
        {
          *d++ = *p++;  /* literal \ or / */
           //++p;  /* literal \ or / */
        }
        else  /* just check for a symbol available */
          if (*(p + 1) == '\0')
          {
            strcpy(pCtx->sError, sInvalidEsc);
            return 6;  /* no symbol ... this is bad */
          }
      *d++ = *p++;
      continue;
    }
    if (bExpectExtended)
      if (*p == '/')
        break;
    *d++ = *p++;
  }
  *d = '\0';
  if (!bExpectExtended)
    goto _export_pattern;
  if (*p != '/')
  {
    strcpy(pCtx->sError, sInvalidPatternTerm);
    return 3;  /* invalid termination of search pattern */
  }
  ++p;
  if (bExplicitMatch)  /* 'm' or 's' was specified at the start of the pattern */
  {
    if (!bReplace)
      goto _process_options;
  }
  /*
  Try to detect valid replace pattern.
  We look for pattern in format /text/replace/. We are at the second '/'
  character. We'll look toward the end of the pattern to see whether
  if terminates with another '/', which will indate a full /text/replace/ pattern
  */
  save_p = p;  /* save the source pointer */
  while (*p)
  {
    if (*p == '/')
    {
      p = save_p;  /* restore the source pointer */
      goto _process_replace_portion;
    }
    if (*p == '\\')
      ++p;  /* skip the escaped character */
    ++p;  /* advance to the next character to peek */
  }
  /* Here: no other '/' characters which determines the decision
  that this is not a complete /text/replace/ pattern. Then we may
  only have some options remaining for processing */
  p = save_p;
  goto _process_options;
  /*
  Extract the replace pattern, look for pattern references (\1, \2, \3 ...)
  */
_process_replace_portion:
  d = sReplace;
  bStartReplPattern = TRUE;
  while (*p)
  {
    if (*p == '/')
      break;
    if (*p == '\\')
    {
      if (isdigit(*(p + 1)))
      {
        ASSERT(pCtx->bRegularExpr);
        *d = '\0';
        pCtx->Repl[nRepl].type = 0;  /* string */
        strcpy(pCtx->Repl[nRepl].data.sReplString, sReplace);
        ++nRepl;
        d = sReplace;
        if (*(p + 1) == '0')
        {
          strcpy(pCtx->sError, sInvalidRefNumber);
          return 4;  /* error, should be >=1 */
        }
        pCtx->Repl[nRepl].type = 1;  /* pattern# */
        pCtx->Repl[nRepl].data.pattern = *(p + 1) - '0';
        ++nRepl;
        p += 2;
        continue;
      }
      else
      {
        if (*p == '\\' && *(p + 1) == 'n')
        {
          p += 2;  /* skip over '\n' */
          *d++ = '\n';
          continue;
        }
        if (*(p + 1) == '\\' || *(p + 1) == '/')
         ++p;  /* literal \ or / */
      }
    }
    *d++ = *p++;
  }
  if (*p != '/')
  {
    strcpy(pCtx->sError, sInvalidReplPatternTerm);
    return 2;  /* invalid termination of replace pattern */
  }
  ++p;
_process_options:
  while (*p)
  {
    if (*p != '\0')
    {
      switch (*p)
      {
        case 'g':
          break;

        default:
          sprintf(pCtx->sError, sInvalidPatternOption);
          return 1;  /* invalid option */
      }
    }
    ++p;
  }
  *d = '\0';
  if (bStartReplPattern)
  {
    pCtx->Repl[nRepl].type = 0;  /* string */
    strcpy(pCtx->Repl[nRepl].data.sReplString, sReplace);
    ++nRepl;
  }
_export_pattern:
  pCtx->nNumRepl = nRepl;
  if (!NewSearch(pCtx, sSearch, nNumLines))
  {
    sprintf(pCtx->sError, "regexp: %s", pCtx->psRegExprError);
    return 7;  /* invalid regular expression pattern */
  }
  return 0;
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

