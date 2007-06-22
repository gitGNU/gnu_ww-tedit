/*

File: hypertvw.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 4th March, 2001
Descrition:
  Hypertext viewer.

*/

#ifndef HYPERTVW_H
#define HYPERTVW_H

#include "clist.h"
#include "keyset.h"

#define MAX_INFO_PAGE_TEXT 4096

/* The text of the page */
typedef struct _InfoPageText
{
  TListEntry link;
  #ifdef _DEBUG
  BYTE MagicByte;
  #endif
  char Text[MAX_INFO_PAGE_TEXT];
  int nBytesRead;
  int nBlockPos;
} TInfoPageText;

#define INFOTEXT_MAGIC 0x57
#ifdef _DEBUG
#define VALID_INFOTEXTBLOCK(pstBlock)  ((pstBlock) != NULL && (pstBlock)->MagicByte == INFOTEXT_MAGIC)
#else
#define VALID_INFOTEXTBLOCK(pstBlock) (1)
#endif

/* To maintain consystency if a page is to be redisplayed */
typedef struct _InfoPageContext
{
  /* Position to go to upon initial display of the page */
  int nDestTopLine;
  int nDestCol;
  int nDestRow;

  int nCol;
  int nRow;
  int nLinePos;  /* Start of the current line */
  int nCurPos;  /* position in the line of nCol cursor position */
  int nTopLine;
  int nTopLinePos;
  int nWrtEdge;
  int nPos;
  int nOldCol;
  int nOldRow;
  int nPrevCol;
  int nPrevRow;
  int nMenuLinePos;  /* caches the position where menu starts */
  int nDir;  /* GetNextChar() advancement directio */
  BOOLEAN bAtALink;  /* the cursor is walking over a link */
  BOOLEAN bEOP;  /* end of page */
  int nLastPos;  /* end of page position */
  char cPrev;
  BOOLEAN bUpdatePage;
  BOOLEAN bUpdateLine;
  BOOLEAN bQuit;
  BOOLEAN bPreserveIncrementalSearch;
  BOOLEAN bIncrementalSearch;
  BOOLEAN bPreserveSelection;
  TListRoot *blist;
  TInfoPageText *pstBlock;  /* Caches the last block */
  char *psLink;
  int nMaxLinkSize;
  /* Block parameters */
  BOOLEAN bBlock;
  int nStartLine;
  int nEndLine;
  int nStartPos;  /* For column block this is the column pos of the left edge */
  int nEndPos;  /* For column block this is the column pos of the right edge */
  BOOLEAN bColumn;
  /* Marking block transition parameters */
  int nExpandCol;
  int nExpandRow;
  int nLastExtendCol;
  int nLastExtendRow;
  int nExtendAncorCol;
  int nExtendAncorRow;
  int x1;  /* Inclusive range */
  int y1;
  int x2;
  int y2;
  int nWidth;

  /* call-back interface:
  pCmdKeyMap - keys/commands pairs
  pfnNavProc - call this function with user context pContext
  and pass the InfoPageContext and the Cmd ID from pCmdKeyMap
  that activated this command.
  if the function returns 0, menas normal exit, no extra actions necessary
  -1 - exit NavigatePage.
  */
  struct KeySequence *pCmdKeyMap;
  int (*pfnNavProc)(struct _InfoPageContext *pCtx, int nCmd);
  void *pContext;  /* extract context for pfnNavProc() */
  dispc_t *disp;
} TInfoPageContext;

void ComposeHyperViewerKeySet(TKeySet *pMainKeySet);
int NavigatePage(TInfoPageContext *pCtx);

#endif  /* HYPERTVW_H */

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

