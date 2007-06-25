/*

File: search.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 18th October, 1999
Descrition:
  Functions concerning text search and replace.

*/

#ifndef SEARCH_H
#define SEARCH_H

/* todo: move this to wlimits.h */
#define MAX_SUBPATTERN_LEN 80
#define MAX_MULTIPLELINE_SEARCH 10

typedef struct ReplaceElement
{
  int type;  /* 0 -string, 1 -pattern_reference */
  union
  {
    int pattern;
    char sReplString[MAX_SUBPATTERN_LEN];
  } data;
} TReplaceElement;

typedef struct SearchContext
{
  #ifdef _DEBUG
  #define SEARCHCTX_MAGIC 0x56
  BYTE MagicByte;
  #endif

  /* All the subpatterns are extracted here
  after a match is found */
  char SubstringBufs[MAX_MULTIPLELINE_SEARCH][MAX_SUBPATTERN_LEN];
  char *Substrings[MAX_MULTIPLELINE_SEARCH];  /* usually points at SubstringBufs */

  /* The replace pattern is broken to
  substrings and reference numbers here */
  TReplaceElement Repl[20];  /* space worth of about 10 refs + 10 substrings */
  int nNumRepl;  /* number of elements in the replace pattern */

  char sSearch[MAX_SEARCH_STR];
  int nNumLines;  /* how much lines are presented in sSearch */
  int nDirection;
  BOOLEAN bCaseSensitive;
  BOOLEAN bPrompt;  /* Replace with prompt */

  /* Occurence result here */
  BOOLEAN bSuccess;  /* Search result */
  int nSearchLine;  /* This is a starting point as well */
  int nSearchPos;
  int nEndLine;
  int nEndPos;
  BOOLEAN bPassedEndOfFile;
  BOOLEAN bPosOnly;  /* The pattern matches a position only (like ^ or $) */
  int nRowOfFirstOccurrence;
  int nColOfFirstOccurrence;
  int nFileID;  /* the file ID where we last did a search */

  /* If we have multi-line search pattern we need to
  extract and concat the correspondent number of lines from the file
  in a buffer */
  char sTextBuf[MAX_TEXTBUF];
  char *psTextBuf;
  BOOLEAN bHeap;  /* psTextBuf points to a block in the heap */
  int psTextBufLen;  /* for diagnostics */

  BOOLEAN bRegularExpr;

  BOOLEAN bRegAllocated;
  void *pstRegExprData;
  char sError[80];  /* suitable error message */
  char *psRegExprError;  /* Error message returned by the regular expression compiler */
  int nErrorOffset;  /* in sSearch[] */
  char nError;  /* only by Search, not by RegExprSearch() */
} TSearchContext;

#ifdef _DEBUG
#define VALID_PSEARCHCTX(p) ((p) != NULL && (p)->MagicByte == SEARCHCTX_MAGIC)
#else
#define VALID_PSEARCHCTX(p) (1)
#endif

void InitSearchContext(TSearchContext *pstSearchContext);
void DoneSearchContext(TSearchContext *pstSearchContext);
void ClearSearchContext(TSearchContext *pstSearchContext);
BOOLEAN NewSearch(TSearchContext *pstSearchContext, const char *sText,
  int nNumLines);
void IncrementalSearch(TFile *pFile, char *c, int nWidth,
  TSearchContext *pstSearchContext);
void IncrementalSearchRemoveLast(TFile *pFile, int nWidth,
  TSearchContext *pstSearchContext);
void _CmdIncrementalSearch(TFile *pFile, int nDir, int nWidth, 
  TSearchContext *pstSearchContext);
BOOLEAN Find(TFile *pFile, int nDir, int nWidth, TSearchContext *pstSearchContext);
BOOLEAN Replace(TSearchContext *pstSearchCtx, TFile *pFile);

const char *ExtractComponent(const char *psPos, char *psDest, BOOLEAN *pbQuoted);
int ParseSearchPattern(const char *sPattern, TSearchContext *pCtx);

#endif /* ifndef SEARCH_H */

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

