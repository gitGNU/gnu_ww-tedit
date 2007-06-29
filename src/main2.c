/*

File: main2.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 6th November, 1998
Descrition:
  Console application work space. Initial setup of application. Main loop.
  Termination of the application.

*/

#include "global.h"
#include "heapg.h"
#include "memory.h"
#include "path.h"
#include "scr.h"
#include "keydefs.h"
#include "kbd.h"
#include "l1def.h"
#include "wlimits.h"
#include "cmd.h"
#include "keyset.h"
#include "l2disp.h"
#include "filecmd.h"
#include "ini.h"
#include "ini2.h"
#include "options.h"
#include "doctype.h"
#include "file2.h"
#include "undo.h"
#include "blockcmd.h"
#include "smalledt.h"
#include "hypertvw.h"
#include "wrkspace.h"
#include "helpcmd.h"
#include "search.h"
#include "searchf.h"
#include "infordr.h"
#include "menudat.h"
#include "palette.h"
#include "c_syntax.h"
#include "py_syntax.h"
#include "edinterf.h"
#include "nav.h"
#include "mru.h"
#include "main2.h"

#include <time.h>

const int vermaj = 0;
const int vermin = 2;
const int ver_revision = 3;
const int verbld = 50;
const int vercfg = 5;
const char *datebld = __DATE__;
const char *timebld = __TIME__;

char sModuleFileName[_MAX_PATH];
char sModulePath[_MAX_PATH];  /* Always with a trailing slash '\\' */
BOOLEAN bQuit = FALSE;
int nINIVersion = 0;  /* Read what is the config version as read from INI file */
int prog_argc;
char **prog_argv;

/* ************************************************************************
   Function: MoveToFileView
   Description:
     Find the current file in pFileInMemoryList and
     activate its view to be on focus.
   Returns:
     FALSE -- no files active, move failed
*/
BOOLEAN MoveToFileView(void)
{
  TView *pView;
  TFile *pCurrentFile;

  pCurrentFile = GetCurrentFile();
  pView = FileListFindFView(pFilesInMemoryList, pCurrentFile);
  if (pView == NULL)  /* no files on the screen, perhaps we are closing app */
    return FALSE;
  ASSERT(VALID_PVIEW(pView));
  ContainerSetFocus(&stRootContainer, pView->pContainer);
  return TRUE;
}

typedef struct
{
  TContainer *pFirstDocked;
  int nNumDocked;
} TDockedCtx;

/* ************************************************************************
   Function: CheckForDocked
   Description:
*/
static int CheckForDocked(TContainer *pCont, void *pCtx)
{
  TDockedCtx *pDockedCtx;

  ASSERT(VALID_PCONTAINER(pCont));

  pDockedCtx = pCtx;
  if (pCont->pView == NULL)
    return 1;
  if (pCont->pView->bDockedView)
  {
    if (pDockedCtx->nNumDocked == 0)
      pDockedCtx->pFirstDocked = pCont;
    ++pDockedCtx->nNumDocked;
  }
  return 1;
}

/* ************************************************************************
   Function: GetADockedView
   Description:
     Returns true if at least one view with bDockedView flag set is active
     on the screen.
*/
static TContainer *GetADockedView(void)
{
  TDockedCtx DockedCtx;
  DockedCtx.nNumDocked = 0;
  DockedCtx.pFirstDocked = NULL;
  ContainerWalkTree(&stRootContainer, CheckForDocked, &DockedCtx);
  return DockedCtx.pFirstDocked;
}

/* ************************************************************************
   Function: CloseDockedView
   Description:
*/
static void CloseDockedView(TContainer *pDockedView)
{
  if (bQuit)
    return;  /* out last breath */

  if (pCurrentContainer == pDockedView)
  {
    if (!MoveToFileView())
      return;
  }
  ContainerCollapse(pDockedView);
}

/* ************************************************************************
   Function: ActivateDocumentOptions
   Description:
     Activates the proper document options depending on the current
     file name extention.
*/
void ActivateDocumentOptions(void)
{
  TDocType *pDocType;
  TFile *pFile;

  if (GetNumberOfFiles() > 0)
  {
    /*
    Detect the type of the new file that is to be put on screen
    and activate the corespondent options.
    */
    pFile = GetCurrentFile();
    pDocType = DetectDocument(DocumentTypes, pFile->sFileName);
    if (pDocType != NULL)
    {
      SetDocumentOptions(pDocType);
      pFile->nType = pDocType->nType;
    }
    else
    {
      SetDocumentOptions(&DefaultDocOptions);
      pFile->nType = tyPlain;
    }
  }
}

/* ************************************************************************
   Function: OnFileSwitched
   Description:
     Called whenever a new file is switched on top.
*/
void OnFileSwitched(TFile *pLastFile)
{
  TFile *pFile;
  TView *pView;

  ActivateDocumentOptions();
  /*
  Move the file at the top of MRUList.
  This orders files to be store in the INI file.
  */
  if (GetNumberOfFiles() > 0)
  {
    pFile = GetCurrentFile();
    AddFileToMRUList(pMRUFilesList, pFile->sFileName, pFile->nCopy, FALSE, TRUE,
      pFile->bForceReadOnly, pFile->nCol, pFile->nRow, pFile->nTopLine, FALSE);
    /*
    pFilesInMemoryList has new file on the top, so we need to establish
    this file view in the current container
    */
    pView = FileListFindFView(pFilesInMemoryList, pFile);
    ASSERT(pView != NULL);
    ContainerSetView(pCurrentContainer, pView);
    pFile->bUpdatePage = TRUE;

    /* Set the window title */
    SetTitle( pFile->sTitle );
  }
}

/* ************************************************************************
   Function: ScrollInsteadOfTooltip
   Description:
     Example case: Cursor is moving up one line and hits the opening
     '{' of a function, because the function name and parameters are not
     visible this will generate a tooltip. Tooltip can be avoided if
     pFile->nTopLine is moved accordingly.
*/
BOOLEAN ScrollInsteadOfTooltip(TFile *pFile, TBracketBlockTooltip *pTooltip)
{
  int nTopLine;

  if (pTooltip->NumLines == 0)
    return FALSE;
  if (pTooltip->bIsTop && pTooltip->bCursorIsOverOpenBracket)
  {
    /*
    Cursor is at '{', scroll up to show the block operator
    */
    pFile->nTopLine -= pTooltip->NumLines;
    return TRUE;
  }
  nTopLine = pFile->nTopLine;
  if (pFile->nRow < nTopLine)
    nTopLine = pFile->nRow;
  if (pTooltip->bIsTop && !pTooltip->bCursorIsOverOpenBracket &&
    nTopLine + pTooltip->NumLines > pFile->nRow)
  {
    /*
    Cursor is at '}', scroll up to show the block operator which
    is at the other end (the '{')
    */
    pFile->nTopLine = pFile->nRow - pTooltip->NumLines;
    return FALSE;  /* tooltip + scroll */
  }
  if (!pTooltip->bIsTop && !pTooltip->bCursorIsOverOpenBracket)
  {
    /*
    Cursor is at '}' and the block operator of interest is at the bottom
    Scroll down
    */
    pFile->nTopLine += pTooltip->NumLines;
    return TRUE;
  }
  return FALSE;
}

/* ************************************************************************
   Function: OnCursorPosChanged
   Description:
     Called whenever a cursor changed position after executing a command
     on the file.
   Note:
     This function is invoked in smalledt.c as well.
     This function is invoked from within fview.c.
*/
void OnCursorPosChanged(TFile *pFile)
{
  TEditInterf stInterf;
  BOOLEAN bBlockSave;
  TIsOverBracket pfnIsOverBracket;
  TSynHRegion regions[6];
  int i;
  int nOldLen;
  int nOldCol;
  int nOldRow;
  TBracketBlockTooltip stTooltip;

  bBlockSave = pFile->bBlock;

  if (!bPersistentBlocks)
  {
    if (!pFile->bPreserveSelection)
    {
      if (pFile->bBlock)
        pFile->bUpdatePage = TRUE;
      pFile->bBlock = FALSE;
      /* Set invalid values for all the block markers */
      #if 0  /* To keep working Ctrl+KB, Ctrl+KK keys */
      pFile->nStartLine = -1;
      pFile->nEndLine = -1;
      pFile->nStartPos = -1;
      pFile->nEndPos = -2;
      #endif
      pFile->nExtendAncorCol = -1;
      pFile->nExtendAncorRow = -1;
      pFile->nExpandCol = -1;
      pFile->nExpandRow = -1;
    }

    if (bBlockSave)
    {
      if (!pFile->bBlock)  /* We have unmarked the current selecion */
        pFile->blockattr &= ~COLUMN_BLOCK;  /* Column block mode is nonpersistent */
    }
  }
  pFile->bPreserveSelection = FALSE;
  pFile->bUpdateStatus = TRUE;

  pfnIsOverBracket = GetIsOverBracketProc(pFile->nType);
  if (pfnIsOverBracket != NULL)
  {
    ProduceEditInterf(&stInterf, pFile);
    /* attemp to highligh { or } that have been just typed */
    if (pFile->bHighlighAreas)  /* goto the pos of the character */
      GotoColRow(pFile, pFile->nCol - 1, pFile->nRow);
    if (pFile->sTooltipBuf == NULL)  /* First time? */
    {
      pFile->nTooltipBufSize = 1024;
      pFile->sTooltipBuf = s_alloc(pFile->nTooltipBufSize);
    }
    stTooltip.pDestBuf = pFile->sTooltipBuf;
    stTooltip.BufSize = pFile->nTooltipBufSize;

    pfnIsOverBracket(regions, _countof(regions), &stTooltip, &stInterf);

    if (!ScrollInsteadOfTooltip(pFile, &stTooltip))
    {
      pFile->bTooltipIsTop = stTooltip.bIsTop;
      pFile->nNumTooltipLines = stTooltip.NumLines;
    }

    ASSERT(_countof(regions) == _countof(pFile->hlightareas));
    nOldLen = pFile->hlightareas[0].l;
    nOldRow = pFile->hlightareas[0].r;
    nOldCol = pFile->hlightareas[0].c;
    for (i = 0; i < _countof(regions); ++i)
    {
      pFile->hlightareas[i].c = regions[i].nPos;
      pFile->hlightareas[i].r = regions[i].nLine;
      pFile->hlightareas[i].l = regions[i].nLen;
    }
    if ( pFile->hlightareas[0].l != nOldLen  /* highlighting area changed? */
         || pFile->hlightareas[0].c != nOldCol
         || pFile->hlightareas[0].r != nOldRow
       )
    {
      pFile->bUpdatePage = TRUE;  /* then redraw the whole page */
      if (!pFile->bBlock)  /* If there is no already a block */
      {
        /* Create a hidden block between the the two brackets */
        nOldCol = pFile->nCol;
        nOldRow = pFile->nRow;
        GotoColRow(pFile, 0, pFile->hlightareas[0].r + 1);
        BeginBlockExtend(pFile);
        GotoColRow(pFile, 0, pFile->hlightareas[1].r);
        EndBlockExtend(pFile);
        pFile->bPreserveSelection = FALSE;
        ToggleBlockHide(pFile);
        GotoColRow(pFile, nOldCol, nOldRow);
      }
    }
    if (pFile->bHighlighAreas)  /* restore cursor pos */
      GotoColRow(pFile, pFile->nCol + 1, pFile->nRow);
    pFile->bHighlighAreas = FALSE;
  }
}

/* ************************************************************************
   Function: OnKeySetChanged
   Description:
    Last key set changed.
    Put here code to be executed when key sequence is changed.
*/
void OnKeySetChanged(void)
{
  PutShortCutKeys();
  ComposeSmallEditKeySet(&KeySet);
  ComposeHyperViewerKeySet(&KeySet);
}

/* ************************************************************************
   Function: StoreRecoveryRecord2
   Description:
     A call-back function for FileListForEach().
     Stores recovery record for a specific file.
     Call StoreRecovery() from undo.c
*/
static BOOLEAN StoreRecoveryRecord2(TFile *pFile, void *pContext)
{
  ASSERT(VALID_PFILE(pFile));

  pFile->bRecoveryStored = StoreRecoveryRecord(pFile);
  return TRUE;
}

#if 0
/* ************************************************************************
   Function: GetTime
   Description:
     Gets the present time.
*/
static void GetTime(TTime *pTime)
{
  struct tm *newtime;
  time_t long_time;

  time(&long_time);  /* Get time as long integer. */
  newtime = localtime(&long_time);
  pTime->year = newtime->tm_year;
  pTime->month = newtime->tm_mon;
  pTime->day = newtime->tm_mday;
  pTime->hour = newtime->tm_hour;
  pTime->min = newtime->tm_min;
  pTime->sec = newtime->tm_sec & 0xfe;  /* 0xfe to mask the odd seconds */
}
#endif

int nNumberOfCommands;

/* ************************************************************************
   Function: HandleEscOnDockedViews
   Description:
    The rule of ESC:
    If in a docked view, exit to a file view.
    If in a file view and a docked view is open close the container.
    If in a file view and no docked views execute the menu (return FALSE here)
*/
static BOOLEAN HandleEscOnDockedViews(void)
{
  TContainer *pDockedView;

  if (pCurrentContainer->pView->bDockedView)
  {
    MoveToFileView();
    return TRUE;
  }
  pDockedView = GetADockedView();
  if (pDockedView != NULL)
  {
    CloseDockedView(pDockedView);
    return TRUE;
  }
  return FALSE;
}

/* ************************************************************************
   Function: CloseTheDockedView
   Description:
*/
void CloseTheDockedView(void)
{
  TContainer *pDockedView;

  if (pCurrentContainer->pView != NULL)
    if (pCurrentContainer->pView->bDockedView)
      MoveToFileView();
  pDockedView = GetADockedView();
  if (pDockedView != NULL)
    CloseDockedView(pDockedView);
}

/* ************************************************************************
   Function: HandleMenu
   Description:
     Event is mouse or key. Returns true if event is handled
     and a command is executed.
*/
static BOOLEAN HandleMenu(struct event *pEvent)
{
  int nCmdCode;
  struct event stEvent;

  nCmdCode = ExecuteMenu(pEvent->e.nKey);
  if (nCmdCode >= 0)
  {
    stEvent.e.nParam = nCmdCode;
    ContainerHandleEvent(MSG_EXECUTE_COMMAND, &stRootContainer, &stEvent);
    return TRUE;
  }
  if (nCmdCode == -3) /* menu was activated, no command was selected */
    return TRUE;  /* key event was handled, no further distribution necessary */
  return FALSE;
}

/* ************************************************************************
   Function: InitDisplaySystem
   Description:
     Call functions for initial setup of the display system.
*/
static BOOLEAN InitDisplaySystem(void)
{
  if (!OpenConsole())
  {
    printf("OpenConsole() failed\n");
    return FALSE;
  }

  if (!MapPalette(CPalette, MAX_PALETTE))
  {
    printf("MapPalette() failed\n");
    CloseConsole();
    return FALSE;
  }

  GetScreenMetrix();
  EnableResize();

  SaveUserScreen();

  return TRUE;
}

static TTime LastWriteTime;

void workspc_get_module_path(int argc, char *argv[])
{
  char sDummy[_MAX_PATH];

  #ifdef WIN32
  GetModuleFileName(NULL, sModuleFileName, _MAX_PATH);
  #else
  strcpy(sModuleFileName, argv[0]);
  #endif
  FSplit(sModuleFileName, sModulePath, sDummy, "ERROR", TRUE, TRUE);
}

/* ************************************************************************
   Function: InitAndRestoreWorkspace
   Description:
     Init workspace module.
     Load option files (master .ini file and the local ww.ini)
     Restore the last workspace by reloading the
     files of the last session.
     pMasterINIFile must already be opened and used to load
     the screen geometry (very early in main())
*/
static BOOLEAN InitAndRestoreWorkspace(char *argv[], TFile *pMasterINIFile)
{
  TFile *pINIFile;

  PrepareHelpFileName();

  nNumberOfCommands = SortCommands(Commands);

  if (InitDocTypes() != 0)
    return FALSE;
  InitWorkspace();

  LoadMasterOptions(pMasterINIFile, FALSE);
  memset(&LastWriteTime, 0, sizeof(LastWriteTime));
  GetFileParameters(pMasterINIFile->sFileName, NULL, &LastWriteTime, NULL);

  pINIFile = CreateINI(sINIFileName);  /* Allocate an empty file structure */
  if (!OpenINI(pINIFile))  /* Load the INI file contents */
    return FALSE;
  LoadWorkspace(pINIFile);
  CloseINI(pINIFile);

  DocTypesSnapshot();  /* Before the plugins! */

  CLangRegister();  /* Registers C/C++ language syntax highlighting */
  PyLangRegister();  /* Registers Python language syntax highlighting */

  RestoreMRUFiles(pFilesInMemoryList, pMRUFilesList, DocumentTypes);
  if (bNoMemory)
  {
    bNoMemory = FALSE;
    ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK, NULL, sNoMemoryMRU);
  }

  if (GetNumberOfFiles() == 0)
    CmdFileNew(NULL);

  OnFileSwitched(NULL);  /* For first time for the file on top */
  AllocKeyBlock();  /* All pressed keys are transformed to TBlocks here */

  HelpPushContext(&stCtxHelp, "LAST_HELP", "~L~ast ovserved page", NULL);
  HelpPushContext(&stCtxHelp, "TOP_HELP", "Main ~H~elp page", NULL);

  OnKeySetChanged();

  return TRUE;
}

/* ************************************************************************
   Function: ProcessCommandLineParameters
   Description:
*/
static void ProcessCommandLineParameters(int argc, char **argv)
{
  int i;

  ProcessCommandLine(argc, argv);
  if (bNoMemory)
  {
    bNoMemory = FALSE;
    ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK, NULL, sNoMemory);
  }

  /*
  Add all the parameters from the command line to file_open_history
  */
  for (i = 1; i < argc; ++i)
    AddHistoryLine(pFileOpenHistory, argv[i]);
}

#if 0
/* ************************************************************************
   Function: ShowTipsAndTricks
   Description:
     Tips and tricks should be shown only once per day.
     We show tips and tricks or we show what's new when this is a new
     build.
     The master ini file keep the version of the last build.
     The master ini file modification timestamp shows wether we
     start the editor for the first time today.
*/
static void ShowTipsAndTricks()
{
  TTime Today;
  BOOLEAN bShownToday;
  char sBuf[_MAX_PATH];

  bShownToday = FALSE;
  GetTime(&Today);
  if (Today.year == LastWriteTime.year &&
    Today.month == LastWriteTime.month &&
    Today.day == LastWriteTime.day)
    bShownToday = TRUE;  /* we have already started the editor at least once since 0am */

  if (nLastBuildAnounced == verbld)
    if (bShownToday)
      return;  /* we have this shown today */

  if (nLastBuildAnounced >= 0 && nLastBuildAnounced < verbld)
  {
    nLastBuildAnounced = verbld;
    sprintf(sBuf, "build%d", verbld);
    if (InfoPageExists(sHelpFile, sBuf))
      NavigateInfoPage(sHelpFile, sBuf, TRUE);
  }
  else
    if (nLastTip >= 0)
    {
      sprintf(sBuf, "tip%d", nLastTip + 1);
      if (InfoPageExists(sHelpFile, sBuf))
      {
        NavigateInfoPage(sHelpFile, sBuf, TRUE);
        ++nLastTip;
      }
    }

  StoreMasterOptions(FALSE);  /* update the ID of the tip shown in the global .ini */
}
#endif

#ifdef WIN32
extern void InstallWinClipboardMonitor(void);
extern void UninstallWinClipboardMonitor(void);
#endif

/* ************************************************************************
   Function: InstallClipboardMonitor
   Description:
*/
static void InstallClipboardMonitor(void)
{
  #ifdef WIN32
  InstallWinClipboardMonitor();
  #endif
}

/* ************************************************************************
   Function: UninstallClipboardMonitor
   Description:
*/
static void UninstallClipboardMonitor(void)
{
  #ifdef WIN32
  UninstallWinClipboardMonitor();
  #endif
}

/* ************************************************************************
   Function: UpdateDisplay
   Description:
*/
static void UpdateDisplay(void)
{
  ContainerHandleEvent(MSG_UPDATE_SCR, &stRootContainer, NULL);
  ContainerHandleEvent(MSG_UPDATE_STATUS_LN, &stRootContainer, NULL);
}

static void (*pfnMouseProc)(struct event *pEvent, void *pContext);
static void *pContext;

/* ************************************************************************
   Function: HandleEvent
   Description:
*/
static void HandleEvent(struct event *pEvent)
{
  struct event stEvent;

  if (pEvent->t.code == EVENT_RESIZE)
  {
    ContainerHandleEvent(pEvent->t.code, &stRootContainer, pEvent);
    goto _exit;
  }

  if (bCtrlReleased)
    ZeroFileListDepth(pFilesInMemoryList);

  switch (pEvent->t.code)
  {
    case EVENT_RECOVERY_TIMER_EXPIRED:
      FileListForEach(pFilesInMemoryList, StoreRecoveryRecord2, FALSE, NULL);
      break;

    case EVENT_KEY:
      if (!bIncrementalSearch)
      {
        if (pEvent->e.nKey == KEY(0, kbEsc))
        {
          if (pfnMouseProc != NULL)  /* Emit "cancel mouse capture" */
          {
            stEvent.t.msg = MSG_RELEASE_MOUSE;
            pfnMouseProc(&stEvent, pContext);
            break;
          }
          if (HandleEscOnDockedViews())
            break;
        }
        if (HandleMenu(pEvent))
          break;
      }
      ContainerHandleEvent(pEvent->t.code, &stRootContainer, pEvent);
      break;

    case EVENT_MOUSE:
      if (pfnMouseProc != NULL)
      {
        pfnMouseProc(pEvent, pContext);
        break;
      }
      ContainerHandleEvent(pEvent->t.code, &stRootContainer, pEvent);
      break;

    default:
      ContainerHandleEvent(pEvent->t.code, &stRootContainer, pEvent);
      break;
  }

  if (GetNumberOfFiles() == 0 && !bQuit)
    ASSERT(0);  /* "noname" file should be displayed when no more files */

_exit:
  if (bNoMemory)
  {
    bNoMemory = FALSE;
    ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK, NULL, sNoMemory);
  }
}

/* ************************************************************************
   Function: SetMouseProc
   Description:
     Sets specific function to grab mouse events.
*/
void SetMouseProc(void (*_pfnMouseProc)(struct event *pEvent, void *pContext),
  void *_pContext)
{
  pfnMouseProc = _pfnMouseProc;
  pContext = _pContext;
}

/* ************************************************************************
   Function: SanityChecks
   Description:
*/
static void SanityChecks(void)
{
  TFile *pFile;
  int nPos;
  TLine *pLine;

  VALIDATE_HEAP();
  pFile = GetCurrentFile();
  CHECK_VALID_UNDO_LIST(pFile);
  ASSERT(pFile->nUndoLevel == 0);  /* On valid end of last cmd */
  if (GetNumberOfFiles() == 0)
    ASSERT(0);  /* "noname" or file from the command line! */
  if (pFile->nRow < pFile->nNumberOfLines)
  {
    ASSERT(pFile->pCurPos != NULL);
    pLine = GetLine(pFile, pFile->nRow);
    nPos = pFile->pCurPos - pLine->pLine;
    ASSERT(IS_VALID_CUR_POS(pFile));
    ASSERT(nPos >= 0);
  }
}

/* ************************************************************************
   Function: main2
   Description:
*/
void main2(int argc, char **argv)
{
  struct event stEvent;
  TFile *pMasterINIFile;
  BOOLEAN bWrkSpaceInitOK;

  prog_argc = argc;
  prog_argv = argv;
  InitSafetyPool();
  workspc_get_module_path(argc, argv);
  GetINIFileName();
  pMasterINIFile = CreateINI(sMasterINIFileName);  /* Allocate an empty file structure */
  if (!OpenINI(pMasterINIFile))  /* Load the INI file contents */
    return;
  LoadScreenGeometry(pMasterINIFile);
  if (!InitDisplaySystem())
    return;  /* fatal flaw encountered */
  InstallClipboardMonitor();
  bWrkSpaceInitOK = InitAndRestoreWorkspace(argv, pMasterINIFile);
  CloseINI(pMasterINIFile);
  if (!bWrkSpaceInitOK)
    goto _abort1;
  ProcessCommandLineParameters(argc, argv);
  OnFileSwitched(NULL);  /* To activate the options for any cmdln file */
  ContainerHandleEvent(MSG_SET_FOCUS, pCurrentContainer, NULL);
  /*ShowTipsAndTricks();*/ /* temporarely disabled */
  bQuit = FALSE;
  disp_set_resize_handler(HandleEvent);
  while (!bQuit) /* --- Main loop --- */
  {
    #if 1  /* To be turned on for extreme checking purposes! */
    SanityChecks();
    #endif

    UpdateDisplay();
    ReadEvent(&stEvent);
    HandleEvent(&stEvent);
  }
  DisposeKeyBlock();
  RemoveTopMRUItem(pMRUFilesList);  /* the mandatory "noname" */
  StoreWorkspace();  /* Store to the INI file */
  StoreMasterOptions(FALSE);  /* Store to the master INI file */
_abort1:
  UninstallClipboardMonitor();
  DocTypesDone();
  DocTypeSnapshotDispose();
  DoneWorkspace();
  DisposeInfoPagesCache();
  ShowUserScreen();
  DisposeUserScreen();
  DisposeSafetyPool();
  CloseConsole();
  #ifdef HEAP_DBG
  _dbg_print_heap("---Heap at end---\n");
  #endif
}

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2005
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

