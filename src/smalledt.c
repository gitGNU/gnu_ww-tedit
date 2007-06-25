/*

File: smalledt.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 23th April, 2000
Descrition:
  Small editor/viewer.
  One example use if as viewer for FindInFiles results, or as editor for
  comment headers.

*/

#include "global.h"
#include "kbd.h"
#include "scr.h"
#include "keydefs.h"
#include "file.h"
#include "block.h"
#include "undo.h"
#include "search.h"
#include "nav.h"
#include "l2disp.h"
#include "palette.h"
#include "keyset.h"
#include "heapg.h"
#include "memory.h"
#include "l1def.h"
#include "ksetcmd.h"
#include "calccmd.h"
#include "smalledt.h"
#include "wrkspace.h"
#include "winclip.h"

typedef struct _SmallEditCtx
{
  void *pFile;
  BOOLEAN bCommandExecuted;
} TSmallEditCtx;

/*
This array comprises the list of commands whose assigned
key combinations to be extracted from the main editor key
bindings table.
*/
static int nSmallEditCommands[] =
{
  cmEditDeleteBlock,
  cmEditUndo,
  cmEditRedo,
  cmEditDel,
  cmEditBackspace,

  cmEditEnter,

  cmHelpKbd
};

/* Prepare space for all of the commands to have more than one
key binding */
static TKeySequence SmallEditUI[_countof(nSmallEditCommands) * 2];
//static TFile *pFile;
static TKeySequence TerminalKeySeq = {END_OF_KEY_LIST_CODE};
static int nNumberOfCommands;

/* ************************************************************************
   Function:
   Description:
*/
static void SmallEditDeleteBlock(void *pCtx)
{
//  DeleteBlock(pFile);
}

/* ************************************************************************
   Function:
   Description:
*/
static void SmallEditUndo(void *pCtx)
{
//  Undo(pFile);
}

/* ************************************************************************
   Function:
   Description:
*/
static void SmallEditRedo(void *pCtx)
{
//  Redo(pFile);
}

/* ************************************************************************
   Function:
   Description:
*/
static void SmallEditDel(void *pCtx)
{
  TSmallEditCtx *pExecutionCtx;
  TSEInterface *pstCallBackInterface;
  TFileView *pFileView;

  ASSERT(VALID_PCONTAINER(pCurrentContainer));
  ASSERT(VALID_PVIEW(pCurrentContainer->pView));

  pFileView = pCurrentContainer->pView->pViewReserved;
  pstCallBackInterface = pFileView->pReserved;
  ASSERT(VALID_PSE_INTERFACE(pstCallBackInterface));

  pExecutionCtx = pCtx;
  if (pstCallBackInterface->CmdDelete != NULL)
    pExecutionCtx->bCommandExecuted =
      !pstCallBackInterface->CmdDelete(pstCallBackInterface->pCtx);
}

/* ************************************************************************
   Function:
   Description:
*/
static void SmallEditBackspace(void *pCtx)
{
}

/* ************************************************************************
   Function:
   Description:
*/
static void SmallEditEnter(void *pCtx)
{
  TSmallEditCtx *pExecutionCtx;
  TSEInterface *pstCallBackInterface;
  TFileView *pFileView;

  ASSERT(VALID_PCONTAINER(pCurrentContainer));
  ASSERT(VALID_PVIEW(pCurrentContainer->pView));

  pFileView = pCurrentContainer->pView->pViewReserved;
  pstCallBackInterface = pFileView->pReserved;
  ASSERT(VALID_PSE_INTERFACE(pstCallBackInterface));

  pExecutionCtx = pCtx;
  if (pstCallBackInterface->CmdEnter != NULL)
    pExecutionCtx->bCommandExecuted =
      !pstCallBackInterface->CmdEnter(pstCallBackInterface->pCtx);
}

static void CmdSmallEditHelpKbd(void *pCtx);

/* All commands in nSmallEditCommands should be described here */
TCmdDesc SmallEditCommands[] =
{
  {cmEditDeleteBlock, "EditDeleteBlock", SmallEditDeleteBlock},
  {cmEditUndo, "EditUndo", SmallEditUndo},
  {cmEditRedo, "EditRedo", SmallEditRedo},
  {cmEditDel, "EditDel", SmallEditDel},
  {cmEditBackspace, "EditBackspace", SmallEditBackspace},
  {cmEditEnter, "EditEnter", SmallEditEnter},
  {cmHelpKbd, "HelpKbd", CmdSmallEditHelpKbd},
  {END_OF_CMD_LIST_CODE, "End_Of_Cmd_List_Code"}  /* LastItem */
};

/* ************************************************************************
   Function:
   Description:
*/
static void CmdSmallEditHelpKbd(void *pCtx)
{
  DisplayKeys(SmallEditCommands, SmallEditUI);
}

/* ************************************************************************
   Function: ComposeKeySet
   Description:
*/
static int compare_ints(const void *pElem1, const void *pElem2)
{
  const int *i1 = (const int *)pElem1;
  const int *i2 = (const int *)pElem2;

  ASSERT(i1 != NULL);
  ASSERT(i2 != NULL);
  ASSERT(*i1 > 0);
  ASSERT(*i2 > 0);

  return (*i1 - *i2);
}

/* ************************************************************************
   Function: ComposeKeySet
   Description:
     Composes keyset for SmallEdit -- extracts the keycodes for edit
     commands.
     Uses the main key set of the editor.
     The result is in SmallEditUI[].
*/
void ComposeSmallEditKeySet(TKeySet *pMainKeySet)
{
  int i;
  int *pnResult;
  TKeySequence *pKeySeq;

  ASSERT(pMainKeySet != NULL);
  ASSERT(pMainKeySet->pKeySet != NULL);

  pKeySeq = SmallEditUI;
  for (i = 0; i < pMainKeySet->nNumberOfItems; ++i)
  {
    pnResult = bsearch(&pMainKeySet->pKeySet[i].nCode, nSmallEditCommands,
      _countof(nSmallEditCommands), sizeof(int), compare_ints);
    if (pnResult == NULL)
      continue;

    /* Copy this keybinding in our local table */
    memcpy(pKeySeq, &pMainKeySet->pKeySet[i], sizeof(TKeySequence));
    ++pKeySeq;
  }
  /* Put the terminal key sequence to mark the end of the list */
  memcpy(pKeySeq, &TerminalKeySeq, sizeof(TKeySequence));

  nNumberOfCommands = SortCommands(SmallEditCommands);  /* In fact this is necessary only once */
}

/* ************************************************************************
   Function: ExecuteSmallEdtCommand
   Description:
     Executes command from the limited set of commands specific
     of small_editor/bookmarks_views
   Returns:
     Returns FALSE if command nCode is not part of the commands
     specific to the small_editor.
     Returns FALSE if specific small_editor command decides that
     there is nothing to do.
     Returns TRUE if command has been executed.
*/
static BOOLEAN ExecuteSmallEdtCommand(const TCmdDesc *pCommands,
  int nCode, int nNumberOfElements, void *pCtx)
{
  TSmallEditCtx stCtx;

  ASSERT(pCommands == SmallEditCommands);

  stCtx.pFile = pCtx;
  stCtx.bCommandExecuted = FALSE;
  if (!ExecuteCommand(pCommands, nCode, nNumberOfElements, &stCtx))
    return FALSE;
  return stCtx.bCommandExecuted;
}

extern void OnCursorPosChanged(TFile *pFile);

/* ************************************************************************
   Function: SmallEdtSetHandlers
   Description:
     SmallEdt uses HandleFileViewEvents() to handle the events
     of the files.
     HandleFileViewEvents() uses default file actions which are unsuitable
     for the SmallEdt, here some of the action handlers are
     overriden with procedures provided here.
*/
void SmallEdtSetHandlers(TFileView *pFileView)
{
  pFileView->nNumberOfCommands = nNumberOfCommands;
  pFileView->pKeySequences = SmallEditUI;
  pFileView->pCommands = SmallEditCommands;
  pFileView->pfnExecuteCommand = ExecuteSmallEdtCommand;
  //pFileView->pfnUpdateStatusLine = UpdateSmallEdtStatusLine;
  pFileView->nPaletteStart = SmallEditorPaletteStart;
}

/* ************************************************************************
   Function: SmallEdit
   Description:
     Editor/Viewer with a subset of functions.
     This is almost the same modal loop as of main2.c.
     Uses half of the screen.
*/
void SmallEdit(TFile *_pFile, TSEInterface *pstCallBackInterface,
  BOOLEAN bDisplayOnly)
{
#if 0
  DWORD dwKey;
  TKeySequence KeySeq;
  int nCmdCode;
  int nStartY;
  const char *psSaveEOFMsg;

  _pFile->bUpdatePage = TRUE;
  ClearKeySequence(&KeySeq);

  bQuit = FALSE;  /* static for this module */
  pFile = _pFile;
  nStartY = ScreenHeight / 2;

  _pFile->bDisplayChanged = FALSE;
  _pFile->bDisplayReadOnly = FALSE;

  /* 
  Display a separator line
  */
  {
    BYTE buff[512];
    int w = ScreenWidth - 1;
    #if USE_ASCII_BOXES
    memset(buff, '-', 512);
    buff[w + 1] = (BYTE)'\0';  /* End of string marker */
    #else
    memset(buff, 'Ä', 512);
    buff[w + 1] = (BYTE)'\0';  /* End of string marker */
    #endif
    WriteXY((char *)buff, 0, nStartY, GetColor(coSmallEdText));
    ++nStartY;
  }

  psSaveEOFMsg = sEndOfFile;
  sEndOfFile = sNoMoreMessages;
  if (bDisplayOnly)
    sEndOfFile = "";

  while (!bQuit)
  {
    #if 1  /* To be turned on for extreme checking purposes! */
    VALIDATE_HEAP();
    CHECK_VALID_UNDO_LIST(pFile);
    ASSERT(pFile->nUndoLevel == 0);
    #endif
    /*
    Update current page, status line
    and cursor position upon waiting for a key.
    */
    {
      if (pFile->nCol != pFile->nPrevCol ||
        pFile->nRow != pFile->nPrevRow)
        OnCursorPosChanged(pFile);
      pFile->nPrevCol = pFile->nCol;
      pFile->nPrevRow = pFile->nRow;

      ConsoleUpdatePage(pFile, 0, nStartY, ScreenWidth, ScreenHeight - 1,
        SmallEditorPaletteStart);
      if (bDisplayOnly)
      {
        bQuit = TRUE;
        continue;
      }
      ConsoleUpdateStatusLine(pFile);
      GotoXY(0 + pFile->x, nStartY + pFile->y);
    }

    dwKey = ReadKey();

    if (dwKey == 0xffff)
      continue;

    if (dwKey == KEY(0, kbEsc))
      bQuit = TRUE;

    AddKey(&KeySeq, dwKey);

    /* 
    Clear the status line message on the first key pressed 
    */
    {
      pFile->sMsg[0] = '\0';
    }

    if ((nCmdCode = ChkKeySequence(SmallEditUI, &KeySeq)) == -1)
      continue;  /* Still more keys to collect */

    bPreserveIncrementalSearch = FALSE;
    if (nCmdCode == -2)
    {
      if (KeySeq.nNumber == 1)
      {
        ExamineKey(NULL, dwKey);
      }
      goto _end_loop;
    }

    /* Dispatched by KeySeq -> nCmdCode */
    if (pstCallBackInterface != NULL)
    {
      if (nCmdCode == cmEditEnter)
      {
        if (pstCallBackInterface->CmdEnter != NULL)
        {
          bQuit = !pstCallBackInterface->CmdEnter(pstCallBackInterface->pCtx);
          goto _end_loop;
        }
      }
      else
        if (nCmdCode == cmEditDel)
        {
          if (pstCallBackInterface->CmdDelete != NULL)
          {
            pstCallBackInterface->CmdDelete(pstCallBackInterface->pCtx);
            goto _end_loop;
          }
        }
    }
    ExecuteCommand(SmallEditCommands, nCmdCode, nNumberOfCommands, NULL);

_end_loop:
    if (!bPreserveIncrementalSearch)
    {
      if (bIncrementalSearch)
      {
        bIncrementalSearch = FALSE;
        pFile->bPreserveSelection = FALSE;  /* ISearch mode exited */
      }
    }
    ClearKeySequence(&KeySeq);
    if (bNoMemory)
    {
      bNoMemory = FALSE;
      ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK, NULL, sNoMemory);
    }
  }

  if (pKeyBlock != NULL)
    DisposeABlock(&pKeyBlock);

  sEndOfFile = psSaveEOFMsg;
  _pFile->bUpdatePage = TRUE;
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

