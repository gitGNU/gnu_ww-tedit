/*

File: blockcmd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 7th April, 1999
Descrition:
  Commands concerning block manipulation.

*/

#include "global.h"
#include "block.h"
#include "nav.h"
#ifdef WIN32
#include "winclip.h"
#endif
#include "cmdc.h"
#include "blockcmd.h"
#include "search.h"
#include "disp.h"
#include "l2disp.h"
#include "wrkspace.h"
#include "l1def.h"
#include "searcmd.h"
#include "edinterf.h"

/* ************************************************************************
   Function: CmdEditMarkBlockBegin
   Description:
*/
void CmdEditMarkBlockBegin(void *pCtx)
{
  MarkBlockBegin(CMDC_PFILE(pCtx));
}

/* ************************************************************************
   Function: CmdEditMarkBlockEnd
   Description:
*/
void CmdEditMarkBlockEnd(void *pCtx)
{
  MarkBlockEnd(CMDC_PFILE(pCtx));
}

/* ************************************************************************
   Function: CmdEditToggleBlockHide
   Description:
*/
void CmdEditToggleBlockHide(void *pCtx)
{
  ToggleBlockHide(CMDC_PFILE(pCtx));
}

/* ************************************************************************
   Function: CmdEditToggleBlockMarkMode
   Description:
*/
void CmdEditToggleBlockMarkMode(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  bBlockMarkMode = !bBlockMarkMode;
  if (bBlockMarkMode)
    BeginBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditSelectAll
   Description:
*/
void CmdEditSelectAll(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  pCurFile->blockattr &= ~COLUMN_BLOCK;  /* stream block for select-all */
  GotoColRow(pCurFile, 0, 0);
  MarkBlockBegin(pCurFile);
  pCurFile->bPreserveSelection = TRUE;
  GotoColRow(pCurFile, 0, pCurFile->nNumberOfLines);
  MarkBlockEnd(pCurFile);
}

/* ************************************************************************
   Function: CmdEditCharLeftExtend
   Description:
*/
void CmdEditCharLeftExtend(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  CharLeft(pCurFile);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditCharRightExtend
   Description:
*/
void CmdEditCharRightExtend(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  CharRight(pCurFile);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditLineUpExtend
   Description:
*/
void CmdEditLineUpExtend(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  LineUp(pCurFile);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditLineDownExtend
   Description:
*/
void CmdEditLineDownExtend(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  LineDown(pCurFile);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditPageUpExtend
   Description:
*/
void CmdEditPageUpExtend(void *pCtx)
{
  TFile *pCurFile;
  dispc_t *disp;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));
  GotoPageUp(pCurFile, disp_wnd_get_height(disp) - 1);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditPageDownExtend
   Description:
*/
void CmdEditPageDownExtend(void *pCtx)
{
  TFile *pCurFile;
  dispc_t *disp;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));
  GotoPageDown(pCurFile, disp_wnd_get_height(disp) - 1);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditHomeExtend
   Description:
*/
void CmdEditHomeExtend(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  GotoHomePosition(pCurFile);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditEndExtend
   Description:
*/
void CmdEditEndExtend(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  GotoEndPosition(pCurFile);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditTopFileExtend
   Description:
*/
void CmdEditTopFileExtend(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  GotoTop(pCurFile);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditBottomFileExtend
   Description:
*/
void CmdEditBottomFileExtend(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  GotoBottom(pCurFile);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditNextWordExtend
   Description:
*/
void CmdEditNextWordExtend(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  GotoNextWordOrWordEnd(pCurFile);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditPrevWordExtend
   Description:
*/
void CmdEditPrevWordExtend(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  BeginBlockExtend(pCurFile);
  GotoPrevWord(pCurFile);
  EndBlockExtend(pCurFile);
}

/* ************************************************************************
   Function: CmdEditDel
   Description:
*/
void CmdEditDel(void *pCtx)
{
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  if (!bPersistentBlocks)
  {
    if (pCurFile->bBlock)
    {
      DeleteBlock(pCurFile);
      bBlockMarkMode = FALSE;
      return;
    }
  }
  DeleteACharacter(pCurFile);
}

/* ************************************************************************
   Function: CmdEditDeleteBlock
   Description:
*/
void CmdEditDeleteBlock(void *pCtx)
{
  DeleteBlock(CMDC_PFILE(pCtx));
  bBlockMarkMode = FALSE;
}

#if defined(_NON_TEXT) && defined(UNIX)
/* TODO: produce unified interface for clipboards */
#include "xclip.h"
#define XCLIP
#endif

/* ************************************************************************
   Function: CmdEditCut
   Description:
*/
void CmdEditCut(void *pCtx)
{
  Cut(CMDC_PFILE(pCtx), &pClipboard);
  b_clipbrd_owner = TRUE;
  #ifdef XCLIP
  xclipbrd_announce(scr_get_disp_ctx(), TRUE);
  xclipbrd_announce(scr_get_disp_ctx(), FALSE);
  #else
    #ifdef WIN32
    if (pClipboard)
      PutToWindowsClipboard(pClipboard);
    #endif
  #endif
  bBlockMarkMode = FALSE;
}

/* ************************************************************************
   Function: CmdEditCopy
   Description:
*/
void CmdEditCopy(void *pCtx)
{
  Copy(CMDC_PFILE(pCtx), &pClipboard);
  b_clipbrd_owner = TRUE;
  #ifdef XCLIP
  xclipbrd_announce(scr_get_disp_ctx(), TRUE);
  xclipbrd_announce(scr_get_disp_ctx(), FALSE);
  #else
    #ifdef WIN32
    if (pClipboard)
      PutToWindowsClipboard(pClipboard);
    #endif
  #endif
  bBlockMarkMode = FALSE;
}

/* ************************************************************************
   Function: CmdEditPaste
   Description:
*/
void CmdEditPaste(void *pCtx)
{
  #ifdef XCLIP
  if (!b_clipbrd_owner)
    xclipbrd_request(scr_get_disp_ctx(), TRUE);
  else
  {
    PasteAndIndent(pCtx, pClipboard);
    bBlockMarkMode = FALSE;
  }
  #else
    #ifdef WIN32
    GetFromWindowsClipboard(&pClipboard);
    #endif
    PasteAndIndent(CMDC_PFILE(pCtx), pClipboard);
    bBlockMarkMode = FALSE;
  #endif
}

/* ************************************************************************
   Function: CmdEditClipboardHistory
   Description:
*/
void CmdEditClipboardHistory(void *pCtx)
{
  #ifdef WIN32
  if (!SelectClipboardHistory(&pClipboard))
    return;
  CmdEditPaste(CMDC_PFILE(pCtx));
  #endif
}

/* ************************************************************************
   Function: CmdEditSort
   Description:
*/
void CmdEditSort(void *pCtx)
{
  SortCurrentBlock(CMDC_PFILE(pCtx));
  bBlockMarkMode = FALSE;
}

/* ************************************************************************
   Function: CmdEditTabify
   Description:
*/
void CmdEditTabify(void *pCtx)
{
}

/* ************************************************************************
   Function: CmdEditUntabify
   Description:
*/
void CmdEditUntabify(void *pCtx)
{
}

/* ************************************************************************
   Function: CmdEditUppercase
   Description:
*/
void CmdEditUppercase(void *pCtx)
{
}

/* ************************************************************************
   Function: CmdEditLowercase
   Description:
*/
void CmdEditLowercase(void *pCtx)
{
}

/* ************************************************************************
   Function: CmdEditTrimTrailingBlanks
   Description:
*/
void CmdEditTrimTrailingBlanks(void *pCtx)
{
  TFile *pFile;
  int nBlanksRemoved;

  pFile = CMDC_PFILE(pCtx);
  if (!pFile->bBlock)
    return;
  if (!TrimTrailingBlanks(pFile, pFile->nStartLine, pFile->nEndLine, &nBlanksRemoved))
    return;
  sprintf(pFile->sMsg, sBlanksRemoved, nBlanksRemoved);
}

/* ************************************************************************
   Function: CmdEditIndent
   Description:
*/
void CmdEditIndent(void *pCtx)
{
  IndentBlock(CMDC_PFILE(pCtx), 1);
}

/* ************************************************************************
   Function: CmdEditUnindent
   Description:
*/
void CmdEditUnindent(void *pCtx)
{
  UnindentBlock(CMDC_PFILE(pCtx), 1);
}

static TBlock *pKeyBlock;

/* ************************************************************************
   Function: CmdEditTab
   Description:
*/
void CmdEditTab(void *pCtx)
{
  TFile *pCurrentFile;
  char *p;
  int next_tab_col;

  pCurrentFile = CMDC_PFILE(pCtx);
  if (bOverwriteBlocks)
  {
    if (pCurrentFile->bBlock)
    {
      pCurrentFile->bBlock = TRUE;  /* unhide if block was hidden */
      CmdEditIndent(pCtx);
      return;
    }
  }

  if (bInsert)
  {
    if (pKeyBlock == NULL)
      AllocKeyBlock();  /* If failed try again */

    if (pKeyBlock == NULL)
      return;

    if (bUseTabs)
    {
      p = GetBlockLine(pKeyBlock, 0)->pLine;
      *p = '\t';
      Paste(pCurrentFile, pKeyBlock);
    }
    else
    {
      next_tab_col = CalcTab(pCurrentFile->nCol + 1);
      /* are we are the end of the line or at end of the file? */
      if (pCurrentFile->pCurPos == NULL || *(pCurrentFile->pCurPos) == '\0')
        GotoColRow(pCurrentFile, next_tab_col, pCurrentFile->nRow);
      else
        InsertBlanks(pCurrentFile, -1, pCurrentFile->nCol, next_tab_col);
    }
  }
  else
    /*GotoPrevTabPosition()*/;
}

/* ************************************************************************
   Function: CmdEditTabBack
   Description:
*/
void CmdEditTabBack(void *pCtx)
{
  TFile *pCurrentFile;

  pCurrentFile = CMDC_PFILE(pCtx);
  if (bOverwriteBlocks)
    if (pCurrentFile->bBlock)
    {
      CmdEditUnindent(pCurrentFile);
      return;
    }

  /*GotoNextTabPosition();*/
}

/* ************************************************************************
   Function: AllocKeyBlock
   Description:
*/
void AllocKeyBlock(void)
{
  pKeyBlock = MakeBlock(" ", 0);
}

/* ************************************************************************
   Function: DisposeKeyBlock
   Description:
*/
void DisposeKeyBlock(void)
{
  DisposeABlock(&pKeyBlock);
}

/* ************************************************************************
   Function: KeyPreproc
   Description:
*/
void KeyPreproc(TFile *pFile, char p)
{
  TExamineKeyProc pfnExamineKey;
  TEditInterf stInterf;

  pfnExamineKey = GetExamineKeyProc(pFile->nType);
  if (pfnExamineKey != NULL)
  {
    ProduceEditInterf(&stInterf, pFile);
    if (pfnExamineKey(p, &stInterf))
      pFile->bHighlighAreas = TRUE;
  }
}

/* ************************************************************************
   Function: ExamineKey
   Description:
*/
void ExamineKey(void *pCtx, DWORD dwKey, BOOLEAN bAutoIncrementalMode)
{
  char *p;
  char sLastIncSearchText[MAX_SEARCH_STR];
  TFile *pFile;
  dispc_t *disp;

  pFile = CMDC_PFILE(pCtx);
  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (dwKey == 0xffff)  /* timer indication */
    return;

  if (pKeyBlock == NULL)
    AllocKeyBlock();  /* If failed try again */

  if (pKeyBlock == NULL)
    return;

  p = GetBlockLine(pKeyBlock, 0)->pLine;
  *p = ASC(dwKey);

  if (dwKey == KEY(0, kbEsc))
  {
    /*
    If pressing ESC in bInctementalSearch mode return the
    last positions prior conducting the search.
    */
    if (bIncrementalSearch)
    {
      GotoColRow(pFile, nPreISearch_Col, nPreISearch_Row);
      return;
    }
  }

  if (*p == 0 || *p == '\n' || *p == '\r')
    return;

  if (bAutoIncrementalMode && !bIncrementalSearch)
    CmdEditIncrementalSearch(pCtx);

  if (bIncrementalSearch)
  {
    strcpy(sLastIncSearchText, stSearchContext.sSearch);
    IncrementalSearch(pFile, p, disp_wnd_get_width(disp), &stSearchContext);
    bPreserveIncrementalSearch = TRUE;
    if (sLastIncSearchText[0] == '\0' && stSearchContext.sSearch[0] != '\0')
      AddHistoryLine(pFindHistory, stSearchContext.sSearch);
    else
      HistoryCheckUpdate(pFindHistory,
        sLastIncSearchText, stSearchContext.sSearch);
    return;
  }

  KeyPreproc(pFile, *p);

  if (bInsert)
    Paste(pFile, pKeyBlock);
  else
    Overwrite(pFile, pKeyBlock);
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

