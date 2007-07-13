/*

File: fview.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 11st July, 2002
Descrition:
  Implementation of a file view.

*/

#include "global.h"
#include "disp.h"

#include "memory.h"
#include "l2disp.h"
#include "palette.h"
#include "wrkspace.h"
#include "cmdc.h"
#include "keyset.h"
#include "blockcmd.h"
#include "fview.h"

#if 0
#if defined(UNIX) && defined(_NON_TEXT)
#include "xclip.h"
#endif
#endif

#if 0
#include "txtf_rend.h"

struct viewc
{
  #ifdef _DEBUG
  BYTE magic_byte;
  #define VIEWC_MAGIC 0x5e
  #endif

  struct
  {
    int x1;
    int y1;
    int w;
    int h;
  } rect;
  dispc_t *disp;
  int palette_start;
};

/* To be used in ASSERT()! */
#ifdef _DEBUG
#define VALID_VIEWC(view) ((view) != NULL && (view)->magic_byte == VIEWC_MAGIC)
#else
#define VALID_VIEWC(view) (1)
#endif

static void
fv_put_buf(void *_dispc,
           int x, int y, int num_chars, const struct txtf_char_attr *rnd_buf)
{
  struct viewc *view;
  int pass_base;
  int pass_count;
  int i;
  #define PASS_SIZE 20  /* TODO: increase to 1024 after tests! */
  BYTE translate_buf[8192];
  void *buf;

  ASSERT(num_chars > 0);
  ASSERT(x > 0);
  ASSERT(y > 0);
  ASSERT(rnd_buf != NULL);

  view = _dispc;
  ASSERT(VALID_VIEWC(view));

  ASSERT(disp_calc_rect_size(view->disp, PASS_SIZE, 1) > sizeof(translate_buf));

  pass_base = 0;
  buf = translate_buf;
  do
  {
    pass_count = min(PASS_SIZE, num_chars);
    for (i = 0; i < pass_count; ++i)
    {
      disp_cbuf_put_char_attr(view->disp, buf, i,
        rnd_buf[i + pass_base].c,
        rnd_buf[i + pass_base].attr + view->palette_start);
    }

    disp_put_block(view->disp,
      view->rect.x1 + x + pass_base,
      view->rect.y1 + y,
      pass_count, 1,
      buf);

    pass_base += pass_count;
    num_chars -= pass_count;
  }
  while (num_chars > 0);
}

struct txtf_disp_x fv_x = {fv_put_buf};
#endif

extern int nNumberOfCommands;  /* calculated in main2.c */
extern void OnCursorPosChanged(TFile *pFile);  /* main2.c */
extern void OnFileSwitched(dispc_t *disp, TFile *pLastFile);
            /* main2.c */

/* ************************************************************************
   Function: BlockMarkMode1
   Description:
     If the editor is in blockmark mode (F8) this function will
     start a transaction to extend the current block.
*/
static void BlockMarkMode1(TFile *pFile)
{
  if (bBlockMarkMode)
    BeginBlockExtend(pFile);
}

/* ************************************************************************
   Function: BlockMarkMode2
   Description:
     If the editor is in blockmark mode (F8) this function will
     end transaction to extend the current block.
*/
static void BlockMarkMode2(TFile *pFile)
{
  if (bBlockMarkMode)
    EndBlockExtend(pFile);
}

/* ************************************************************************
   Function: StopIncrementalSearch
   Description:
*/
static void StopIncrementalSearch(TFile *pFile)
{
  if (!bPreserveIncrementalSearch)
  {
    if (bIncrementalSearch)
    {
      bIncrementalSearch = FALSE;
      pFile->bPreserveSelection = FALSE;  /* ISearch mode exited */
    }
  }
}

/* ************************************************************************
   Function: HandleFileViewEvents
   Description:
   Returns:
     0 -- continue distributing the event
     1 -- event handled, no further handling necessary
*/
static int HandleFileViewEvents(disp_event_t *ev, TView *pView)
{
  TFileView *pFileView;
  int nCmdCode;
  TFile *pCurFile;
  BOOLEAN (*pfnExecuteCommand)(const TCmdDesc *pCommands,
    int nCode, int nNumberOfElements, void *pCtx);
  void (*pfnExamineKey)(void *pCtx, DWORD dwKey, BOOLEAN bAutoIncrementalMode);
  TKeySequence *pKeySequences;
  int nPaletteStart;
  TContainer *pOwner;
  int ExitCode;
  BOOLEAN bDockedView;
  TExtraColorInterf *pExtraColorInterf;
  #if 0
  #if defined(UNIX) && defined(_NON_TEXT)
  char clipbrd_buf[16 * 1024];
  char *clipbrd_str;
  #endif
  #endif
  wrkspace_data_t *wrkspace;
  dispc_t *disp;
  cmdc_t cmdc;

  wrkspace = ev->data1;
  disp = wrkspace_get_disp(wrkspace);

  ExitCode = 0;

  pOwner = pView->pContainer;
  pFileView = pView->pViewReserved;
  pCurFile = pFileView->pFile;
  bDockedView = pOwner->pView->bDockedView;

  /*
  Use basic or overloaded functions
  */
  pfnExecuteCommand = ExecuteCommand;
  pfnExamineKey = ExamineKey;
  pKeySequences = KeySet.pKeySet;
  nPaletteStart = SyntaxPaletteStart;
  pExtraColorInterf = NULL;
  if (pFileView->pfnExecuteCommand != NULL)
    pfnExecuteCommand = pFileView->pfnExecuteCommand;
  if (pFileView->pfnExamineKey != NULL)
    pfnExamineKey = pFileView->pfnExamineKey;
  if (pFileView->pKeySequences != NULL)
    pKeySequences = pFileView->pKeySequences;
  if (pFileView->nPaletteStart != -1)
    nPaletteStart = pFileView->nPaletteStart;
  if (pFileView->bColorInterfActivated)
    pExtraColorInterf = &pFileView->ExtraColorInterf;

  /* OnCursorPosChanged() should be called for the
  current file and prev cursor position reset */
  if (pCurFile->nCol != pCurFile->nPrevCol ||
    pCurFile->nRow != pCurFile->nPrevRow)
    OnCursorPosChanged(pCurFile);
  pCurFile->nPrevCol = pCurFile->nCol;
  pCurFile->nPrevRow = pCurFile->nRow;

  /* Update nNumVisibleLines */
  pCurFile->nNumVisibleLines = pView->nHeight;

  if (ev->t.code != EVENT_USR)
  {
    switch (ev->t.code)
    {
      case EVENT_KEY:
        /*
        Only the active view may process keyboard events
        */
        if (!pView->bOnFocus)
          break;

        AddKey(&pFileView->KeySeq, ev->e.kbd.key);
        /* Clear the status line message on the first key pressed */
        pCurFile->sMsg[0] = '\0';
        if ((nCmdCode = ChkKeySequence(KeySet.pKeySet, &pFileView->KeySeq)) == -1)
          break;  /* Still more keys to collect */
        bPreserveIncrementalSearch = FALSE;

        if (nCmdCode == -2)
        {
          /*
          Unrecognized in the main program KeySet,
          Lets try the small_edit keyset (the overloaded KeySequences)
          */
          if (pKeySequences != NULL)
          {
            if ((nCmdCode = ChkKeySequence(pKeySequences, &pFileView->KeySeq)) == -1)
              break;  /* Still more keys to collect */
            if (nCmdCode != -2)  /* Recognized there? */
              goto _execute_command;  /* Proceed with the execution */
          }

          if (pFileView->KeySeq.nNumber == 1)
          {
            /*
            Check:
            1. if it is not user typing letters
            2. Menu command (Alt+F)
            3. ESC
            */
            /* TODO: cmd ctx instead of pCurFile */
            CMDC_SET(cmdc, wrkspace, pCurFile);
            pfnExamineKey(&cmdc, ev->e.kbd.key,
              pFileView->bAutoIncrementalSearch);
          }
          ClearKeySequence(&pFileView->KeySeq);  /* no longer needed */
        }
        else  /* This was a recognized file command key sequence */
        {
          TFile *pFile;
          static int nLastFile;  /* ID of the last file */
          static int nLastNumberOfFiles;

          /* Sequence is indentified, flush it out */
          ClearKeySequence(&pFileView->KeySeq);

_execute_command:
          if (pFileView->pfnExecuteCommand != NULL)
          {
            /* This is not a file from pFilesInMemoryList
            but simply a detached file view (SmallEdt) */
            CMDC_SET(cmdc, wrkspace, pFileView->pFile);
            BlockMarkMode1(pFileView->pFile);
            if (!pfnExecuteCommand(pFileView->pCommands, nCmdCode,
              pFileView->nNumberOfCommands, &cmdc))
            {
              BlockMarkMode2(pFileView->pFile);
              goto _execute_command2;
            }
            ExitCode = 1;   /* Command was handled, no further dist. of the event */
            StopIncrementalSearch(pFileView->pFile);
          }
          else
          {
            /* Dispatched by KeySeq -> nCmdCode */
            /* GetCurrentFile() has no meaning if this is the smalledt view */
            /* but executing is will cause no harm */
_execute_command2:
            pFile = GetCurrentFile();
            nLastFile = -1;
            if (pFile != NULL)
              nLastFile = pFile->nID;
            nLastNumberOfFiles = GetNumberOfFiles();

            pFile = pFileView->pFile;
            CMDC_SET(cmdc, wrkspace, pFile);
            BlockMarkMode1(pFile);
            ExecuteCommand(Commands, nCmdCode, nNumberOfCommands, &cmdc);
            BlockMarkMode2(pFile);
            ExitCode = 1;   /* No further dist. of the event */
            if (GetNumberOfFiles() > 0)
              StopIncrementalSearch(pFile);

            /*
            We are in a file (not in a docked view like Bookmarks, FindInFiles)
            */

            /* Check whether the bookmarks window need an update */
            if (stUserBookmarks.bViewDirty)
              BookmarksInvalidate(&stUserBookmarks);

            /* Check whether the file was switched */
            pFile = GetCurrentFile();
            if (pFile == NULL)  /* no more files */
              OnFileSwitched(wrkspace_get_disp(wrkspace), NULL);
            else
            {
              if (nLastFile != pFile->nID)
              {
                /*
                Put here code to be executed whenever the
                current file is switched.
                */
                OnFileSwitched(wrkspace_get_disp(wrkspace),
                               SearchFileListByID(pFilesInMemoryList, nLastFile));

                bBlockMarkMode = FALSE;
                if (nLastNumberOfFiles != GetNumberOfFiles())
                {
                  /*
                  Last file is closed.
                  */
                }
              }
            }
          }
        }

        if (GetNumberOfFiles() > 0)
          StopIncrementalSearch(GetCurrentFile());
        break;

      case EVENT_RESIZE:
        pCurFile->bUpdatePage = TRUE;
        pCurFile->bUpdateStatus = TRUE;  /* We need it to update cursor pos */
        ConsoleUpdatePage(pCurFile,
          pView->x, pView->y,
          pView->x + pView->nWidth, pView->y + pView->nHeight, nPaletteStart,
          pExtraColorInterf, disp);
        if (pFileView->pfnUpdateStatusLine != NULL)
          pFileView->pfnUpdateStatusLine(pFileView->pFile);
        else
          if (ConsoleUpdateStatusLine(pCurFile, disp))
            disp_cursor_goto_xy(disp, pView->x + pCurFile->x, pView->y + pCurFile->y);
        break;

      case EVENT_CLIPBOARD_PASTE:
        if (pClipboard != NULL)
          DisposeABlock(&pClipboard);
        pClipboard = MakeBlock(ev->e.pdata, 0);
        PasteAndIndent(GetCurrentFile(), pClipboard);
        bBlockMarkMode = FALSE;
        /* TODO: implement this function accross platforms and remove #ifdefs */
        #if 0
        #if defined(UNIX) && defined(_NON_TEXT)
        xclipbrd_free(pEvent->e.pdata);
        #endif
        #endif
        break;

      case EVENT_CLIPBOARD_CLEAR:
        b_clipbrd_owner = FALSE;
        break;

      case EVENT_CLIPBOARD_COPY_REQUESTED:
        #if 0
        #if defined(UNIX) && defined(_NON_TEXT)
        clipbrd_str = tblock_copy_to_str(pClipboard, DEFAULT_EOL_TYPE,
          clipbrd_buf, sizeof(clipbrd_buf));
        if (clipbrd_str == NULL)
          break;
        xclipbrd_send(scr_get_disp_ctx(),
          pEvent->e.p.param1, pEvent->e.p.param3,
          pEvent->e.p.param4, pEvent->e.p.param5,
          clipbrd_str, pEvent->e.p.param2);
        if (clipbrd_str != clipbrd_buf)  /* allocated in the heap? */
          s_free(clipbrd_str);
        #endif
        #endif
        break;
    }
  }
  else
  {
    switch (ev->t.user_msg_code)
    {
      case MSG_EXECUTE_COMMAND:
        nCmdCode = ev->e.param;
        goto _execute_command;

      case MSG_UPDATE_SCR:
#if 0
        {
          struct txtf *f = pCurFile;
          struct txtf_edit_flags fflags =
            {bSyntaxHighlighting, bMatchPairHighlighting, nTabSize};
          struct viewc view =
          {
            #ifdef _DEBUG
            VIEWC_MAGIC,
            #endif
            {pView->x, pView->y, pView->nWidth, pView->nHeight},
            0, /*dispc_t *disp; TODO: pass via wrkspc struct */
            nPaletteStart
          };
          txtf_show(f, &fflags, &fv_x, &view, pView->nWidth, pView->nHeight);
        }
#endif
        #if 1
        ConsoleUpdatePage(pCurFile,
          pView->x, pView->y,
          pView->x + pView->nWidth, pView->y + pView->nHeight, nPaletteStart,
          pExtraColorInterf, disp);
        #endif
        break;

      case MSG_UPDATE_STATUS_LN:
        /*
        Only the active view may update the status line
        */
        if (!pView->bOnFocus)
          break;

        if (pFileView->pfnUpdateStatusLine != NULL)
          pFileView->pfnUpdateStatusLine(pFileView->pFile);
        else
          if (ConsoleUpdateStatusLine(pCurFile, disp))
            disp_cursor_goto_xy(disp, pView->x + pCurFile->x, pView->y + pCurFile->y);
        break;

      case MSG_INVALIDATE_SCR:
        pCurFile->bUpdatePage = TRUE;
        pCurFile->bUpdateStatus = TRUE;
        break;

      case MSG_SET_MIN_SIZE:
        pOwner->nMinWidth = 4;
        pOwner->nMinHeight = 4;
        break;


      case MSG_SET_FOCUS:
        pView->bOnFocus = TRUE;
        pCurFile->bUpdateStatus = TRUE;
        pCurFile->bShowBlockCursor = FALSE;
        pCurFile->bUpdateLine = TRUE;  /* no block cursor to take effect */
        break;

      case MSG_KILL_FOCUS:
        pView->bOnFocus = FALSE;
        pCurFile->bShowBlockCursor = TRUE;
        pCurFile->bUpdateLine = TRUE;  /* no block cursor to take effect */
        break;
    }
  }

  return ExitCode;
}

/* ************************************************************************
   Function: FileViewInit
   Description:
*/
void FileViewInit(TView *pView, TFile *pFile, TFileView *pFileView)
{
  pView->pfnHandleEvent = HandleFileViewEvents;
  pView->pViewReserved = pFileView;

  pFileView->pFile = pFile;
  ClearKeySequence(&pFileView->KeySeq);

  /* Use the defaults for these */
  pFileView->pKeySequences = NULL;
  pFileView->pCommands = NULL;
  pFileView->pfnExamineKey = NULL;
  pFileView->pfnExecuteCommand = NULL;
  pFileView->nNumberOfCommands = 0;
  pFileView->pfnUpdateStatusLine = NULL;
  pFileView->nPaletteStart= -1;
  pFileView->pReserved = NULL;
  pFileView->bColorInterfActivated = FALSE;
  memset(&pFileView->ExtraColorInterf, 0, sizeof(pFileView->ExtraColorInterf));
  memset(pFileView->FuncNameCtx, 0, sizeof(pFileView->FuncNameCtx));
  pFileView->bAutoIncrementalSearch = FALSE;
}

void FileViewDone(TFileView *pFileView)
{
}

typedef struct _FindFViewCtx
{
  TFileView *pFileView;
  TContainer *pResult;
} TFindFViewCtx;

/* ************************************************************************
   Function: CheckForVFiew
   Description:
     Call back function of the ContainerWalTree() called by
     FileViewFindContainer.
     Checks whether the current container contains a file view
     (by checking what is the function that handle the events)
     and then checks whether the file container is not the container
     we search for.
*/
static int CheckForVFiew(TContainer *pCont, void *_pCtx)
{
  TFindFViewCtx *pCtx;

  pCtx = _pCtx;
  if (pCont->pView == NULL)
    return 1;  /* continue the search */
  if (pCont->pView->pfnHandleEvent != HandleFileViewEvents)
    return 1;  /* continue the search */
  if (pCont->pView->pViewReserved == pCtx->pFileView)
  {
    pCtx->pResult = pCont;
    return 0;  /* Terminate the search */
  }
  return 1;  /* continue the search */
}

/* ************************************************************************
   Function: FileViewFindContainer
   Description:
     Finds the container that holds specific pFileView.
     Returns NULL if not container on the screen contains the specific file
     view.
*/
TContainer *FileViewFindContainer(TFileView *pFileView)
{
  TFindFViewCtx stCtx;

  stCtx.pFileView = pFileView;
  stCtx.pResult = NULL;
  ContainerWalkTree(&stRootContainer, CheckForVFiew, &stCtx);
  return stCtx.pResult;
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

