/*

File: bookmcmd.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 17th September, 2000
Descrition:
  Bookmarks manipulation commands.

*/

#include "global.h"
#include "l1def.h"
#include "disp.h"
#include "wrkspace.h"
#include "nav.h"
#include "smalledt.h"
#include "bookm.h"
#include "cmdc.h"
#include "bookmcmd.h"
#include "contain.h"
#include "infordr.h"
#include "memory.h"
#include "searchf.h"
#include "filenav.h"

/* Some interface functions necessary for SmallEditor */
static TSEInterface stCallBackInterface;
static TSEInterface stBookmarksSmallEdtInterface;
static TSEInterface stFindInFiles1SmallEdtInterface;
static TSEInterface stFindInFiles2SmallEdtInterface;
static TSEInterface stOutputSmallEdtInterface;
static TSEInterface stFuncCallBackInterf;
static BOOLEAN bActivateBookmark;

/* ************************************************************************
   Function: CmdEditBookmarkToggle
   Description:
     Togglse the bookmark on/off for the current line.
     [Ctrl+F2]
*/
void CmdEditBookmarkToggle(void *pCtx)
{
  TFile *pFile;
  char *psContent;
  TMarkLocation *pMark;

  pFile = CMDC_PFILE(pCtx);
  pMark = BMListCheck(pFile->sFileName, pFile->nRow, &stUserBookmarks);
  if (pMark != NULL)
  {
    BMListRemoveBookmark(pMark);
  }
  else
  {
    psContent = GetLineText(pFile, pFile->nRow);
    BMListInsert(pFile->sFileName, pFile->nRow, 0, -1,
      psContent, NULL, 0, &stUserBookmarks, NULL, 0);
  }
  BookmarksInvalidate(&stUserBookmarks);
  pFile->bUpdatePage = TRUE;  /* and show the bookmark on the screen */
}

/* ************************************************************************
   Function: ActivateBookmark
   Description:
     A call back function that is invoked whenever <Enter> is pressed
     over a bookmark line in the viewer.
*/
static BOOLEAN ActivateBookmark(void *_pCtx)
{
  TBookmarksSet *pCtx;

  pCtx = _pCtx;
  BMSetActivateBookmark(pCtx, pCtx->bSetIsSorted, pCtx->pActivateBookmarksCtx);
  return FALSE;  /* Info to viewer: you are becoming inactive */
}

/* ************************************************************************
   Function: DeleteBookmark
   Description:
     A call back function that is invoked whenever <Del> is pressed
     over a bookmark line in the viewer.

     Removes single bookmark or a selection of bookmarks.
*/
static BOOLEAN DeleteBookmark(void *pCtx)
{
  TBookmarksSet *pBMSet;
  TFile *pViewer;
  int i;
  TBMFindNextCtx BMCtx;
  TMarkLocation *pPrevMark;
  TMarkLocation *pMark;
  int Dummy;
  int nNumberOfLines;
  int nStartLine;
  disp_event_t ev;

  pBMSet = pCtx;
  ASSERT(VALID_PBOOKMARKSSET(pBMSet));
  pViewer = &pBMSet->Viewer;
  ASSERT(VALID_PFILE(&pBMSet->Viewer));

  nNumberOfLines = 1;
  nStartLine = pViewer->nRow;
  if (!bPersistentBlocks)
  {
    if (pViewer->bBlock)
    {
      nNumberOfLines = pViewer->nEndLine - pViewer->nStartLine;
      nStartLine = pViewer->nStartLine;
    }
  }
  if (nNumberOfLines < 1)
    nNumberOfLines = 1;
  pPrevMark = BMSetFindFirstBookmark(pBMSet, NULL, 0, nStartLine, &BMCtx, &Dummy);
  for (i = 0; i < nNumberOfLines; ++i)
  {
    if (pPrevMark == NULL)
      break;
    pMark = BMSetFindNextBookmark(&BMCtx, &Dummy);
    BMListRemoveBookmark(pPrevMark);
    pPrevMark = pMark;
  }

  BookmarksInvalidate(pBMSet);  /* Redraw on the screen */

  disp_event_clear(&ev);
  ev.t.code = EVENT_USR;
  ev.t.user_msg_code = MSG_INVALIDATE_SCR;
  ContainerHandleEvent(&stRootContainer, &ev); /* +allfiles*/
  return TRUE;
}

/* ************************************************************************
   Function: ActivateBMSetOnTheScreen
   Description:
     Activates the last bookmark set.
     If the correspondent view is not activated then splits the screen
     and inserts bookmarks set view into the newly created container.
*/
static void ActivateBMSetOnTheScreen(TBookmarksSet *pBMSet)
{
  TContainer *pCont;
  disp_event_t ev;

  ASSERT(VALID_PBOOKMARKSSET(pBMSet));

  /*
  Case1: the last BMSet on the screen was the BMSet to reqested for activation
  */
  if (pstLastBMSet == pBMSet)
  {
    if (pBMSet->stView.pContainer != NULL)
    {
      /* View is already on the screen. Set this to be the current containter */
      ContainerSetFocus(&stRootContainer, pBMSet->stView.pContainer);
      return;
    }

_new:
    pMessagesContainer = ContainerSplit(&stRootContainer,
      &pBMSet->stView, 500, TRUE);
    pBMSet->Viewer.bUpdatePage = TRUE;  /* update on the screen */
    pBMSet->Viewer.bUpdateStatus = TRUE;
    if (pMessagesContainer != NULL)
      ContainerSetFocus(&stRootContainer, pMessagesContainer);
    pstLastBMSet = pBMSet;
    return;
  }

  /*
  Case2: New BMSet on the screen (should split screen) or
  replace existing BMSet
  */
  if (pstLastBMSet == NULL)  /* First time BMSet is shown */
    goto _new;
  pCont = pstLastBMSet->stView.pContainer;
  if (pCont == NULL) /* Tha last BMSet was hidden from the screen */
    goto _new;
  /* still on the screen */
  ContainerRemoveView(pCont);
  ContainerSetView(pCont, &pBMSet->stView);

  disp_event_clear(&ev);
  ev.t.code = EVENT_USR;
  ev.t.user_msg_code = MSG_INVALIDATE_SCR;
  ContainerHandleEvent(pCont, &ev);

  ContainerSetFocus(&stRootContainer, pBMSet->stView.pContainer);
  pstLastBMSet = pBMSet;
}

/* ************************************************************************
   Function: CmdWindowMessages
   Description:
     Activates the last window displied in the modal viewer.
*/
void CmdWindowMessages(void *pCtx)
{
  if (pstLastBMSet == NULL)  /* called for first time?! */
    CmdWindowBookmarks(NULL);  /* render user bookmarks by default */
  RenderBookmarks(pstLastBMSet);
  ActivateBMSetOnTheScreen(pstLastBMSet);
}

/* ************************************************************************
   Function: CmdWindowBookmarks
   Description:
     Display a modal viewer of the bookmarks.

  TODO: FindInFiles1&2 and HelpHistory should use their
  own TSEInterfaces established through pViewerInterface
*/
void CmdWindowBookmarks(void *pCtx)
{
  /*
  Prepare the contents of the viewer
  (SmallEdt uses stUserBookmarks.stViewer as a read-only file
  to represent the bookmarks on the screen)
  */
  RenderBookmarks(&stUserBookmarks);

  /*
  stUserBookmarks is passed as a context to our Activate/Delete functions
  and transmits information about what is the Bookmarkset
  */
  stUserBookmarks.pViewerInterface = &stBookmarksSmallEdtInterface;
  stUserBookmarks.bSetIsSorted = FALSE;

  /*
  stBookmarksSmallEdtInterface is used by the SmallEditorEventHandler
  to perform bookmark related activities -- activate or delete
  */
  stBookmarksSmallEdtInterface.CmdEnter = ActivateBookmark;
  stBookmarksSmallEdtInterface.CmdDelete = DeleteBookmark;
  stBookmarksSmallEdtInterface.pCtx = &stUserBookmarks;
  #ifdef _DEBUG
  stBookmarksSmallEdtInterface.MagicByte = SE_INTERFACE_MAGIC;
  #endif
  stUserBookmarks.stFileView.pReserved = &stBookmarksSmallEdtInterface;

  /*
  We need to override some of the default
  stUserBookmarks.stFileView action handlers
  */
  SmallEdtSetHandlers(&stUserBookmarks.stFileView);

  /*
  We are ready for the show:
  Make the bookmarks window appear as a top view
  */
  ActivateBMSetOnTheScreen(&stUserBookmarks);

  /*
  pstLastBMSet is used by CmdWindowMessages to reactivate the
  last bookmarks window
  */
}

/* ************************************************************************
   Function: CmdWindowFindInFiles1
   Description:
     Display a modal viewer of the Find In File results
*/
void CmdWindowFindInFiles1(void *pCtx)
{
  /*
  Prepare the contents of the viewer
  (SmallEdt uses stFindInFiles1.stViewer as a read-only file
  to represent the bookmarks on the screen)
  */
  RenderBookmarks(&stFindInFiles1);

  /*
  stFindInFiles1 is passed as a context to our Activate/Delete functions
  and transmits information about what is the Bookmarkset
  */
  stFindInFiles1.pViewerInterface = &stFindInFiles1SmallEdtInterface;
  stFindInFiles1.bSetIsSorted = TRUE;

  /*
  stFindInFiles1SmallEdtInterface is used by the SmallEditorEventHandler
  to perform bookmark related activities -- activate or delete
  */
  stFindInFiles1SmallEdtInterface.CmdEnter = ActivateBookmark;
  stFindInFiles1SmallEdtInterface.CmdDelete = NULL;
  stFindInFiles1SmallEdtInterface.pCtx = &stFindInFiles1;
  #ifdef _DEBUG
  stFindInFiles1SmallEdtInterface.MagicByte = SE_INTERFACE_MAGIC;
  #endif
  stFindInFiles1.stFileView.pReserved = &stFindInFiles1SmallEdtInterface;

  /*
  We need to override some of the default
  stFindInFiles1.stFileView action handlers
  */
  SmallEdtSetHandlers(&stFindInFiles1.stFileView);

  /*
  We are ready for the show:
  Make the bookmarks window appear as a top view
  */
  ActivateBMSetOnTheScreen(&stFindInFiles1);

  /*
  pstLastBMSet is used by CmdWindowMessages to reactivate the
  last bookmarks window
  */
}

/* ************************************************************************
   Function: CmdWindowFindInFiles2
   Description:
     Display a modal viewer of the Find In File results
*/
void CmdWindowFindInFiles2(void *pCtx)
{
  /*
  Prepare the contents of the viewer
  (SmallEdt uses stFindInFiles2.stViewer as a read-only file
  to represent the bookmarks on the screen)
  */
  RenderBookmarks(&stFindInFiles2);

  /*
  stFindInFiles2 is passed as a context to our Activate/Delete functions
  and transmits information about what is the Bookmarkset
  */
  stFindInFiles2.pViewerInterface = &stFindInFiles2SmallEdtInterface;
  stFindInFiles2.bSetIsSorted = TRUE;

  /*
  stFindInFiles2SmallEdtInterface is used by the SmallEditorEventHandler
  to perform bookmark related activities -- activate or delete
  */
  stFindInFiles2SmallEdtInterface.CmdEnter = ActivateBookmark;
  stFindInFiles2SmallEdtInterface.CmdDelete = NULL;
  stFindInFiles2SmallEdtInterface.pCtx = &stFindInFiles2;
  #ifdef _DEBUG
  stFindInFiles2SmallEdtInterface.MagicByte = SE_INTERFACE_MAGIC;
  #endif
  stFindInFiles2.stFileView.pReserved = &stFindInFiles2SmallEdtInterface;

  /*
  We need to override some of the default
  stFindInFiles2.stFileView action handlers
  */
  SmallEdtSetHandlers(&stFindInFiles2.stFileView);

  /*
  We are ready for the show:
  Make the bookmarks window appear as a top view
  */
  ActivateBMSetOnTheScreen(&stFindInFiles2);

  /*
  pstLastBMSet is used by CmdWindowMessages to reactivate the
  last bookmarks window
  */
}

/* ************************************************************************
   Function: CmdWindowOutput
   Description:
     Displays viewer of Output window
*/
void CmdWindowOutput(void *pCtx)
{
  /*
  Prepare the contents of the viewer
  (SmallEdt uses stOutput.stViewer as a read-only file
  to represent the bookmarks on the screen)
  */
  RenderBookmarks(&stOutput);

  /*
  stOutput is passed as a context to our Activate/Delete functions
  and transmits information about what is the Bookmarkset
  */
  stOutput.pViewerInterface = &stOutputSmallEdtInterface;
  stOutput.bSetIsSorted = TRUE;

  /*
  stOutputSmallEdtInterface is used by the SmallEditorEventHandler
  to perform bookmark related activities -- activate or delete
  */
  stOutputSmallEdtInterface.CmdEnter = ActivateBookmark;
  stOutputSmallEdtInterface.CmdDelete = NULL;
  stOutputSmallEdtInterface.pCtx = &stOutput;
  #ifdef _DEBUG
  stOutputSmallEdtInterface.MagicByte = SE_INTERFACE_MAGIC;
  #endif
  stOutput.stFileView.pReserved = &stOutputSmallEdtInterface;

  /*
  We need to override some of the default
  stOutput.stFileView action handlers
  */
  SmallEdtSetHandlers(&stOutput.stFileView);

  /*
  We are ready for the show:
  Make the bookmarks window appear as a top view
  */
  ActivateBMSetOnTheScreen(&stOutput);

  /*
  pstLastBMSet is used by CmdWindowMessages to reactivate the
  last bookmarks window
  */
}

/* ************************************************************************
   Function: CmdEditSearchFunctions
   Description:
*/
void CmdEditSearchFunctions(void *pCtx)
{
  TBookmarksSet *pstFuncNames;
  TFile *pCurFile;

  pCurFile = CMDC_PFILE(pCtx);
  PrepareFunctionNamesBookmarks(pCurFile);

  /*
  Prepare the contents of the viewer
  (SmallEdt uses stFindInFiles1.stViewer as a read-only file
  to represent the bookmarks on the screen)
  */
  pstFuncNames = GetFuncNamesBMSet(pFilesInMemoryList, pCurFile);
  RenderBookmarks(pstFuncNames);

  /*
  pstFuncNames is passed as a context to our Activate/Delete functions
  and transmits information about what is the Bookmarkset
  */
  pstFuncNames->pViewerInterface = &stFuncCallBackInterf;
  pstFuncNames->bSetIsSorted = FALSE;

  /*
  stFuncCallBackInterf is used by the SmallEditorEventHandler
  to perform bookmark related activities -- activate or delete
  */
  stFuncCallBackInterf.CmdEnter = ActivateBookmark;
  stFuncCallBackInterf.CmdDelete = NULL;
  stFuncCallBackInterf.pCtx = pstFuncNames;
  #ifdef _DEBUG
  stFuncCallBackInterf.MagicByte = SE_INTERFACE_MAGIC;
  #endif
  pstFuncNames->stFileView.pReserved = &stFuncCallBackInterf;
  pstFuncNames->stFileView.ExtraColorInterf.pExtraCtx =
    pstFuncNames->stFileView.FuncNameCtx;
  ASSERT(sizeof(TBMFindNextCtx) < sizeof(pstFuncNames->stFileView.FuncNameCtx));
  pstFuncNames->stFileView.ExtraColorInterf.pReserved1 = pstFuncNames;
  pstFuncNames->stFileView.ExtraColorInterf.pfnApplyExtraColors =
    (void *)ApplyFuncBookmarksColors;
  pstFuncNames->stFileView.bColorInterfActivated = TRUE;
  pstFuncNames->stFileView.bAutoIncrementalSearch = TRUE;

  /*
  We need to override some of the default
  pstFuncNames->stFileView action handlers
  */
  SmallEdtSetHandlers(&pstFuncNames->stFileView);

  /*
  We are ready for the show:
  Make the bookmarks window appear as a top view
  */
  ActivateBMSetOnTheScreen(pstFuncNames);

  /*
  pstLastBMSet is used by CmdWindowMessages to reactivate the
  last bookmarks window
  */
}

/* ************************************************************************
   Function: CmdWindowTagNext
   Description:
     Operates on the last active bookmarks window (UserBookmarks,
     FindInFiles1, FindInFiles2). Activates the next bookmark in the list.
*/
void CmdWindowTagNext(void *pCtx)
{
  TFile *pFile;
  BOOLEAN bPassedEndOfTagList;
  int nNewRow;

  if (pstLastBMSet == NULL)
    return;
  bPassedEndOfTagList = FALSE;
  nNewRow = pstLastBMSet->Viewer.nRow + 1;
  if (nNewRow >= pstLastBMSet->Viewer.nNumberOfLines)
  {
    GotoColRow(&pstLastBMSet->Viewer, 0, 0);
    bPassedEndOfTagList = TRUE;
  }
  else
    GotoColRow(&pstLastBMSet->Viewer, 0, nNewRow);
  BMSetActivateBookmark(pstLastBMSet, pstLastBMSet->bSetIsSorted, NULL);
  if (bPassedEndOfTagList)
  {
    pFile = pCtx;
    strcpy(pFile->sMsg, sPassedEndOfTagList);
  }
}

/* ************************************************************************
   Function: CmdWindowTagPrev
   Description:
     Operates on the last active bookmarks window (UserBookmarks,
     FindInFiles1, FindInFiles2). Activates the previous bookmark in the list.
*/
void CmdWindowTagPrev(void *pCtx)
{
  TFile *pFile;
  BOOLEAN bPassedEndOfTagList;
  int nNewRow;

  if (pstLastBMSet == NULL)
    return;
  nNewRow = pstLastBMSet->Viewer.nRow - 1;
  bPassedEndOfTagList = FALSE;
  if (nNewRow  < 0)
  {
    GotoColRow(&pstLastBMSet->Viewer, 0,
      pstLastBMSet->Viewer.nNumberOfLines - 1);
    bPassedEndOfTagList = TRUE;
  }
  else
    GotoColRow(&pstLastBMSet->Viewer, 0, nNewRow);
  BMSetActivateBookmark(pstLastBMSet, pstLastBMSet->bSetIsSorted, NULL);
  if (bPassedEndOfTagList)
  {
    pFile = pCtx;
    strcpy(pFile->sMsg, sPassedEndOfTagList);
  }
}

/* ************************************************************************
   Function: CmdWindowBookmarkNext
   Description:
     Operates on the user bookmarks set. Activates the next bookmark in the list.
*/
void CmdWindowBookmarkNext(void *pCtx)
{
  TFile *pFile;
  BOOLEAN bPassedEndOfTagList;
  int nNewRow;
  disp_event_t ev;

  /* As they may be still not rendered and we need the Viewer.nRow field */
  RenderBookmarks(&stUserBookmarks);

  bPassedEndOfTagList = FALSE;
  if (stUserBookmarks.Viewer.nNumberOfLines > 1)
    nNewRow = stUserBookmarks.Viewer.nRow + 1;
  else
    nNewRow = 0;

  bPassedEndOfTagList = FALSE;
  if (nNewRow >= stUserBookmarks.Viewer.nNumberOfLines)
  {
    GotoColRow(&stUserBookmarks.Viewer, 0, 0);
    bPassedEndOfTagList = TRUE;
  }
  else
    GotoColRow(&stUserBookmarks.Viewer, 0, nNewRow);

  BMSetActivateBookmark(&stUserBookmarks, stUserBookmarks.bSetIsSorted, NULL);

  /* NOTE: After BMSetActivateBookmark() we might have pCtx not pointing
  to a valid file. For example, editor is *only* holding single "noname". pCtx
  points to this file. BMSetActivateBookmark() loads a new file and removes
  "noname" from memory, hence pCtx is dangling */

  if (bPassedEndOfTagList)
  {
    pFile = pCtx;  /* This was bad */
    pFile = GetCurrentFile();
    ASSERT(VALID_PFILE(pFile));
    strcpy(pFile->sMsg, sPassedEndOfTagList);

    disp_event_clear(&ev);
    ev.t.code = EVENT_USR;
    ev.t.user_msg_code = MSG_INVALIDATE_SCR;
    ContainerHandleEvent(&stRootContainer, &ev); /* +allfiles*/
  }
}

/* ************************************************************************
   Function: CmdWindowBookmarkPrev
   Description:
     Operates on the user bookmarks set. Activates the previous bookmark in the list.
*/
void CmdWindowBookmarkPrev(void *pCtx)
{
  TFile *pFile;
  BOOLEAN bPassedEndOfTagList;
  int nNewRow;

  /* As they may be still not rendered and we need the Viewer.nRow field */
  RenderBookmarks(&stUserBookmarks);

  nNewRow = stUserBookmarks.Viewer.nRow - 1;
  bPassedEndOfTagList = FALSE;
  if (nNewRow  < 0)
  {
    GotoColRow(&stUserBookmarks.Viewer, 0,
      stUserBookmarks.Viewer.nNumberOfLines - 1);
    bPassedEndOfTagList = TRUE;
  }
  else
    GotoColRow(&stUserBookmarks.Viewer, 0, nNewRow);

  BMSetActivateBookmark(&stUserBookmarks, stUserBookmarks.bSetIsSorted, NULL);
  if (bPassedEndOfTagList)
  {
    pFile = pCtx;
    strcpy(pFile->sMsg, sPassedEndOfTagList);
  }
}

typedef struct _BookmCtx
{
  char InfoFile[_MAX_PATH];
  char InfoPage[MAX_NODE_LEN];
  int nTopLine;
  int nCol;
  int nRow;
} TBookmCtx;

/* ************************************************************************
   Function: ActivateHistoryBookmark
   Description:
     A call-back function to extract from the bookmarks list
     the info file location into the pContext structure.
*/
static BOOLEAN ActivateHistoryBookmark(TMarkLocation *pMark, int nRow, void *pContext)
{
  TBookmCtx *pCtx;

  pCtx = pContext;
  strcpy(pCtx->InfoFile, pMark->pFileName->sFileName);
  strcpy(pCtx->InfoPage, pMark->psContent);
  pCtx->nRow = nRow;
  pCtx->nCol = pMark->nCol;
  pCtx->nTopLine = pMark->nTopLine;
  pMark->bVisited = TRUE;
  return FALSE;
}

/* ************************************************************************
   Function: SelectHelpHistory
   Description:
     Shows the info reader history window. Returns the selection
     in the pDest* parameters. Returns FALSE if nothing selected
     by the user.
     Uses the presorted bookmarkset stInfoHistory. The bookmarks
     are collected by NavigateInfoPage() [infordr.c]. This function
     is activated either by CmdHelpHistory() or by NavigateInfoPage()
*/
BOOLEAN SelectHelpHistory(char *psDestInfoFile, char *psDestInfoPage,
  int *pnDestRow, int *pnDestCol, int *pnDestTopLine)
{
  TBookmCtx stCtx;

  RenderBookmarks(&stInfoHistory);
  stCallBackInterface.CmdEnter = ActivateBookmark;
  stCallBackInterface.CmdDelete = NULL;
  bActivateBookmark = FALSE;

  /* Position the viewer on the top-most line, which is the last
  entry posted in the history */
  stInfoHistory.Viewer.nTopLine = 0;
  GotoColRow(&stInfoHistory.Viewer, 0, 0);

  SmallEdit(&stInfoHistory.Viewer, &stCallBackInterface, FALSE);
  pstLastBMSet = &stInfoHistory;
  if (bActivateBookmark)
  {
    if (stInfoHistory.pfnActivateBookmark == NULL)
      stInfoHistory.pfnActivateBookmark = ActivateHistoryBookmark;
    BMSetActivateBookmark(&stInfoHistory, TRUE, &stCtx);
    strcpy(psDestInfoFile, stCtx.InfoFile);
    strcpy(psDestInfoPage, stCtx.InfoPage);
    *pnDestRow = stCtx.nRow;
    *pnDestCol = stCtx.nCol;
    *pnDestTopLine = stCtx.nTopLine;
    return TRUE;
  }
  return FALSE;
}

/* ************************************************************************
   Function: CmdHelpHistory
   Description:
     Shows the info reader history window.
*/
void CmdHelpHistory(void *pCtx)
{
  char sInfoFile[_MAX_PATH];
  char sInfoPage[_MAX_PATH];
  int nRow;
  int nCol;
  int nTopLine;

  if (!SelectHelpHistory(sInfoFile, sInfoPage, &nRow, &nCol, &nTopLine))
    return;

#if 0
  NavigateInfoPageEx(sInfoFile, sInfoPage, nRow, nCol, nTopLine, FALSE);
#endif
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

