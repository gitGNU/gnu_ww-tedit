/*

File: menudat.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 1st January, 1999
Descrition:
  Description of all the menu items of the editor.
  Menu supporting functions.

*/

#include "global.h"
#include "mru.h"
#include "wrkspace.h"
#include "disp.h"
#include "l2disp.h"
#include "keyset.h"
#include "menu.h"
#include "enterln.h"
#include "ini.h"
#include "file2.h"
#include "options.h"
#include "l1def.h"
#include "defs.h"
#include "main2.h"
#include "umenu.h"
#include "cmd.h"
#include "palette.h"
#include "cmdc.h"
#include "menudat.h"

#define cmNotReady -4
#define cmSetDefaultDoc  -5

/* TODO: rework menu to not use global static disp */
static dispc_t *s_disp;

static void InvokeHelp(int nCmdCode, const char *psPage,
  const char *psCtxHlpItem, const char *psInfoFile);

/* File sub menu items */
static TMenuItem itFileNew = {cmFileNew, 0, "~N~ew", {0}, 0, 0};
static TMenuItem itFileOpen = {cmFileOpen, 0, "~O~pen", {0}, 0, 0};
static TMenuItem itFileOpenAsReadOnly = {cmFileOpenAsReadOnly, 0, "Open As ~R~ead Only", {0}, 0, 0};
static TMenuItem itFileClose = {cmFileClose, 0, "~C~lose", {0}, 0, 0};
static TMenuItem itFileSave = {cmFileSave, 0, "~S~ave", {0}, 0, 0};
static TMenuItem itFileSaveAs = {cmFileSaveAs, 0, "Save ~A~s", {0}, 0, 0};
static TMenuItem itFileSaveAll = {cmFileSaveAll, 0, "Save Al~l~", {0}, 0, 0};
static TMenuItem itFileRecentFiles = {0, 0, "Recent ~F~iles", {0}, 0, 0, "recentfiles", "Help on (File)~R~ecent files"};
static TMenuItem itFileExit = {cmFileExit, 0, "E~x~it", {0}, 0, 0};
static TMenuItem *aFile[] =
{
  &itFileNew, &itFileOpen, &itFileOpenAsReadOnly, &itFileClose, &itSep,
  &itFileSave, &itFileSaveAs, &itFileSaveAll, &itSep,
  &itFileRecentFiles, &itSep,
  &itFileExit
};

static TMenuItem itEditSort = {cmEditSort, 0, "~S~ort", {0}, 0, 0};
static TMenuItem itEditTabify = {cmEditTabify, meDISABLED, "~T~abify", {0}, 0, 0};
static TMenuItem itEditUntabify = {cmEditUntabify, meDISABLED, "U~n~tabify", {0}, 0, 0};
static TMenuItem itEditUppercase = {cmEditUppercase, 0, "~U~ppercase", {0}, 0, 0};
static TMenuItem itEditLowercase = {cmEditLowercase, 0, "~L~owercase", {0}, 0, 0};
static TMenuItem itEditRemoveBlanks = {cmEditTrimTrailingBlanks, 0, "T~r~im trailing blanks", {0}, 0, 0};
static TMenuItem itEditIndent = {cmEditIndent, 0, "~I~ndent", {0}, 0, 0};
static TMenuItem itEditUnindent = {cmEditUnindent, 0, "Unin~d~ent", {0}, 0, 0};
static TMenuItem *aBlock[] =
{
  &itEditSort, &itEditTabify,
  &itEditUntabify, &itEditUppercase,
  &itEditLowercase, &itEditRemoveBlanks,
  &itEditIndent, &itEditUnindent
};
static TMenu meBlock = {MEVERTICAL, _countof(aBlock), (TMenuItem *(*)[])&aBlock, 0};

/* Edit sub menu items */
static TMenuItem itEditUndo = {cmEditUndo, 0, "~U~ndo", {0}, 0, 0};
static TMenuItem itEditRedo = {cmEditRedo, 0, "~R~edo", {0}, 0, 0};
static TMenuItem itEditCut = {cmEditCut, 0, "Cu~t~", {0}, 0, 0};
static TMenuItem itEditCopy = {cmEditCopy, 0, "~C~opy", {0}, 0, 0};
static TMenuItem itEditPaste = {cmEditPaste, 0, "~P~aste", {0}, 0, 0};
static TMenuItem itEditBlock = {0, 0, "~B~lock", {0}, &meBlock, 0, "block", "Help on (Edit)~B~lock operations"};
static TMenuItem itPara = {/*cmWrapPara,*/ cmNotReady, meDISABLED, "~W~rap paragraph", {0}, 0, 0};
static TMenuItem itCenter = {/*cmCenterLn,*/ cmNotReady, meDISABLED, "C~e~nter line", {0}, 0, 0};
static TMenuItem itCommentproc = {/*cmCommentProc,*/ cmNotReady, meDISABLED, "C~o~mment header", {0}, 0, 0};
static TMenuItem *aEdit[] =
{
  &itEditUndo, &itEditRedo,
  &itSep,
  &itEditCut, &itEditCopy, &itEditPaste,
  &itSep,
  &itEditBlock,
  &itSep,
  &itPara, &itCenter, &itCommentproc
};

/* Search sub menu items */
static TMenuItem itFind = {cmEditFind, 0, "~F~ind", {0}, 0, 0};
static TMenuItem itEditFindNext = {cmEditFindNext, 0, "~A~gain", {0}, 0, 0};
static TMenuItem itFindInFiles = {cmEditFindInFiles, 0, "F~i~nd in files", {0}, 0, 0};
static TMenuItem itGotoLine = {cmEditGotoLine, 0, "~G~oto line", {0}, 0, 0};
static TMenuItem itMatch = {/*cmMatch,*/ cmNotReady, meDISABLED, "~M~atch", {0}, 0, 0};
static TMenuItem itSearchFunc = {cmEditSearchFunctions, 0, "F~u~nctions", {0}, 0, 0};
static TMenuItem *aSearch[] = {&itFind, &itEditFindNext,
  &itSep, &itFindInFiles, &itSep,
  &itGotoLine, &itMatch, &itSearchFunc};

/* Tool sub menu items */
static TMenuItem itCalc = {cmToolCalculator, 0, "~C~alculator", {0}, 0, 0};
static TMenuItem itPLFile = {cmToolParseLogFile, 0, "~P~arse Log File", {0}, 0, 0};
static TMenuItem itASCII = {/*cmCalc,*/ cmNotReady, meDISABLED, "ASCII ~T~able", {0}, 0, 0};
static TMenuItem itAddTool = {/*cmAddTool,*/ cmNotReady, meDISABLED, "~A~dd", {0}, 0, 0};
static TMenuItem itEditTool = {/*cmEditTool,*/ cmNotReady, meDISABLED, "~E~dit", {0}, 0, 0};
static TMenuItem itRemoveTool = {/*cmRemoveTool,*/ cmNotReady, meDISABLED, "~R~emove", {0}, 0, 0};
#define MAXTOOLS 10
/*static TMenuItem itTools[MAXTOOLS];*/
static TMenuItem *aTool[7 + 1 + MAXTOOLS] = {&itCalc, &itPLFile, &itASCII, &itSep, &itAddTool,
  &itEditTool, &itRemoveTool};

/* Window sub menu items */
static TMenuItem itWindowNext = {cmNotReady, 0, "~N~ext", {0}, 0, 0};
static TMenuItem itWindowPrev = {cmNotReady, 0, "~P~rev", {0}, 0, 0};
static TMenuItem itWindowSwap = {cmWindowSwap, 0, "~S~wap last", {0}, 0, 0};
static TMenuItem itWindowUserScreen = {cmWindowUserScreen, 0, "~U~ser screen", {0}, 0, 0};
static TMenuItem itWindowList = {cmWindowList, 0, "~L~ist", {0}, 0, 0};
static TMenuItem itWindowMessages = {cmWindowMessages, 0, "~M~essages", {0}, 0, 0};
static TMenuItem itWindowBookmarks = {cmWindowBookmarks, 0, "~B~ookmarks", {0}, 0, 0};
static TMenuItem itWindowFindInFiles1 = {cmWindowFindInFiles1, 0, "F~i~nd in Files 1", {0}, 0, 0};
static TMenuItem itWindowFindInFiles2 = {cmWindowFindInFiles2, 0, "Fin~d~ in Files 2", {0}, 0, 0};
static TMenuItem itWindowOutput = {cmWindowOutput, 0, "~O~utput", {0}, 0, 0};
static TMenuItem *aWindow[35] = {&itWindowNext, &itWindowPrev, &itWindowSwap,
  &itWindowUserScreen, &itWindowList,
  &itSep, &itWindowMessages, &itWindowBookmarks, &itWindowFindInFiles1, &itWindowFindInFiles2,
  &itWindowOutput, &itSep};
#define WINDOWMENUITEMS 12

/* forward definitions */
static void EditDocumentTypeSet(void);
static void EditDefaultDocument(void);
static void SetDefaultDocumentTypeSet(void);
static void EditCurrentDocumentTypeSet(void);

static TMenuItem itOptionsDocumentTypes1 = {0, 0, "~E~dit document type set...", {0}, 0, EditDocumentTypeSet, "opt_doc_set", "Help on (Options)~E~dit document type set"};
static TMenuItem itOptionsDocumentTypes2 = {0, 0, "Default document ~o~ptions", {0}, 0, EditDefaultDocument, "opt_edit_def_doc", "Help on (Options)~E~dit default document"};
static TMenuItem itOptionsDocumentTypes3 = {cmSetDefaultDoc, 0, "Set ~d~efault document set", {0}, 0, 0, "opt_set_def_doc", "Help on (Options)~S~et default document set"};
static TMenuItem *aOptionsDocument[] =
{
  &itOptionsDocumentTypes1, &itOptionsDocumentTypes2, &itOptionsDocumentTypes3
};

static TMenu meOptionsDocument = {MEVERTICAL, _countof(aOptionsDocument), (TMenuItem *(*)[])&aOptionsDocument, 0};

/* forward definitions */
static void InputFileExtention(void);
static void InputFileType(void);
static void ToggleUseTab(void);
static void ToggleOptFill(void);
static void ToggleAutoIndent(void);
static void ToggleBUnind(void);
static void ToggleCTabs(void);
static void InputTabSize(void);
static void ToggleWWrap(void);

static TMenuItem itFExt = {0, 0, "Filename ~e~xtention", {0}, 0, InputFileExtention, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem itFType = {0, 0, "File t~y~pe", {0}, 0, InputFileType, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem itUseTab = {0, 0, "~U~se tab character", {0}, 0, ToggleUseTab, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem itOptFill = {0, 0, "~O~ptimal fill", {0}, 0, ToggleOptFill, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem itAutoInd = {0, 0, "Auto ~i~ndent", {0}, 0, ToggleAutoIndent, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem itBUnind = {0, 0, "Backspace ~u~nindent", {0}, 0, ToggleBUnind, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem itCTabs = {0, 0, "~C~ursor through tabs", {0}, 0, ToggleCTabs, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem itTabSz = {0, 0, "~T~ab size", {0}, 0, InputTabSize, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem itWWrap = {0, 0, "~W~ord wrap", {0}, 0, ToggleWWrap, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem itOK = {1, 0, "[O~K~]", {0}, 0, 0};
static TMenuItem itCancel = {2, 0, "[Cancel - ESC]", {0}, 0, 0};
static TMenuItem *aDocumentOpts[] = {&itFExt, &itFType, &itSep, &itUseTab, &itOptFill,
  &itAutoInd, &itBUnind, &itCTabs, &itTabSz, &itWWrap, &itSep, &itOK, &itCancel};

static TMenu meDocumentOpts = {MEVERTICAL, _countof(aDocumentOpts), (TMenuItem *(*)[])&aDocumentOpts, 0, InvokeHelp, coMenu};

/* forward definitions */
static void ToggleBackup(void);
static void ToggleLastFiles(void);
static void TogglePersistentBlocks(void);
static void ToggleOverwriteBlocks(void);
static void ToggleSyntax(void);
static void ToggleMSyntx(void);
static void ToggleIF0(void);
static void ToggleAscendingSort(void);
static void ToggleCaseSensitiveSort(void);
static void InputRecoveryTime(void);
static void InputRightMargin(void);
static void InputFileSaveMode(void);

static TMenuItem itOptionsBackup = {0, 0, "~B~ackup files", {0}, 0, ToggleBackup, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsLastFiles = {0, 0, "Restore ~l~ast files", {0}, 0, ToggleLastFiles, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsRecoveryTime = {0, 0, "Recovery ~t~ime", {0}, 0, InputRecoveryTime, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsPersistentBlocks = {0, 0, "~P~ersistent blocks", {0}, 0, TogglePersistentBlocks, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsOverwriteBlocks = {0, 0, "~O~verwrite blocks", {0}, 0, ToggleOverwriteBlocks, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsSyntax = {0, 0, "Syntax ~h~ighlighting", {0}, 0, ToggleSyntax, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsMSyntx = {0, 0, "Match ~p~air highlighting", {0}, 0, ToggleMSyntx, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsHighlightIF0 = {0, 0, "Highlight #if ~0~ style comments", {0}, 0, ToggleIF0, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsRMarg = {0, 0, "Right ~m~arging", {0}, 0, InputRightMargin, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsAscendingSort = {0, 0, "~A~scending sort", {0}, 0, ToggleAscendingSort, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsCaseSensitiveSort = {0, 0, "~C~ase sensitive sort", {0}, 0, ToggleCaseSensitiveSort, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsSaveAs = {0, 0, "~F~ile saving mode (end-of-line)", "Auto  ", 0, InputFileSaveMode, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsDocumentTypes4 = {0, 0, NULL, {0}, 0, EditCurrentDocumentTypeSet, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem *aEditOpts[] = {&itOptionsBackup, &itOptionsLastFiles,
  &itOptionsRecoveryTime, &itOptionsPersistentBlocks,
  &itOptionsOverwriteBlocks, &itOptionsSyntax,
  &itOptionsMSyntx, &itOptionsHighlightIF0, &itOptionsRMarg,
  &itOptionsAscendingSort, &itOptionsCaseSensitiveSort,
  &itOptionsSaveAs,
  &itSep,
  &itOptionsDocumentTypes4
};

static TMenu meEditOpts = {MEVERTICAL, _countof(aEditOpts), (TMenuItem *(*)[])&aEditOpts, 0};

/* Options sub menu items */
static TMenuItem itOptionsDocumentTypes = {0, 0, "~D~ocument type set", "   ", &meOptionsDocument, 0, "opt_document", "Help on (Options)~D~ocument options"};
static TMenuItem itOptionsColors = {0, meDISABLED, "~C~olors", {0}, 0, 0};
static TMenuItem itOptionsComment = {0, meDISABLED, "C~o~mment headers", {0}, 0, 0};
static TMenuItem itOptionsKeyboard = {0, meDISABLED, "~K~eyboard", {0}, 0, 0};
static TMenuItem itEditOpt = {0, 0, "~E~dit options", {0}, &meEditOpts, 0, "opt_main", "Help on (Options)~M~ain"};
static TMenuItem itOptionsSave = {cmOptionsSave, 0, "~S~ave options", {0}, 0, 0};
static TMenuItem *aOptions[] = {&itOptionsDocumentTypes,
  &itOptionsColors, &itOptionsComment, &itOptionsKeyboard, &itEditOpt, &itSep,
  &itOptionsSave};

static TMenuItem itHelpEditor = {cmHelpEditor, 0, "~E~ditor help page", {0}, 0, 0};
static TMenuItem itHelpIndex = {cmNotReady, meDISABLED, "~I~ndex", {0}, 0, 0};
static TMenuItem itHelpSearch = {cmNotReady, meDISABLED, "~S~earch", {0}, 0, 0};
static TMenuItem itHelpHistory = {cmHelpHistory, 0, "~H~istory", {0}, 0, 0};
static TMenuItem itHelpBookmarks = {cmNotReady, meDISABLED, "~B~ookmarks", {0}, 0, 0};
static TMenuItem itHelpSubset = {cmNotReady, meDISABLED, "~D~efine subset", {0}, 0, 0};
static TMenuItem itHelpOpen = {cmHelpOpenFile, 0, "~O~pen help file", {0}, 0, 0};
static TMenuItem itHelpKbd = {cmHelpKbd, 0, "~K~eyboard Map", {0}, 0, 0};
static TMenuItem itHelpAbout = {cmHelpAbout, 0, "~A~bout", {0}, 0, 0};
static TMenuItem *aHelp[] =
{
  &itHelpEditor,
  &itHelpIndex,
  &itHelpSearch,
  &itHelpHistory,
  &itHelpBookmarks,
  &itHelpSubset,
  &itHelpOpen,
  &itHelpKbd,
  &itHelpAbout
};

static TMenu meFile = {MEVERTICAL, _countof(aFile), (TMenuItem *(*)[])&aFile, 0};
static TMenu meEdit = {MEVERTICAL, _countof(aEdit), (TMenuItem *(*)[])&aEdit, 0};
static TMenu meSearch = {MEVERTICAL, _countof(aSearch), (TMenuItem *(*)[])&aSearch, 0};
static TMenu meWindow = {MEVERTICAL, 7, (TMenuItem *(*)[])&aWindow, 0};
static TMenu meOptions = {MEVERTICAL, _countof(aOptions), (TMenuItem *(*)[])&aOptions, 0};
static TMenu meTool = {MEVERTICAL, 6, (TMenuItem *(*)[])&aTool, 0};
static TMenu meHelp = {MEVERTICAL, _countof(aHelp), (TMenuItem *(*)[])&aHelp, 0};

static TMenuItem itFile = {0, 0, "~F~ile", {0}, &meFile, 0};
static TMenuItem itEdit = {0, 0, "~E~dit", {0}, &meEdit, 0};
static TMenuItem itSearch = {0, 0, "~S~earch", {0}, &meSearch, 0};
static TMenuItem itTool = {0, 0, "~T~ool", {0}, &meTool, 0};
static TMenuItem itWindow = {0, 0, "~W~indow", {0}, &meWindow, 0};
static TMenuItem itOptions = {0, 0, "~O~ptions", {0}, &meOptions, 0};
static TMenuItem itHelp = {0, 0, "~H~elp", {0}, &meHelp, 0};

static TMenuItem *aRoot[] =
{
  &itFile, &itEdit, &itSearch,
  &itTool, &itWindow, &itOptions,
  &itHelp
};

static TMenu meRoot = {MEHORIZONTAL, _countof(aRoot), (TMenuItem *(*)[])&aRoot, 0, InvokeHelp, coMenu};

/* ************************************************************************
   Function: CmdNotReady
   Description:
*/
static void CmdNotReady(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));
  ConsoleMessageProc(disp, NULL, MSG_ERROR, NULL, "This command is not ready");
}

/* ************************************************************************
   Function: CompareCmdCode
   Description:
     A call-back function used by bsearch().
     Compares two elements of type TKeySequence.
     Only nCode field is significant.
*/
static int CompareCmdCode(const void *p1, const void *p2)
{
  return ((TKeySequence *)p1)->nCode - ((TKeySequence *)p2)->nCode;
}

/* ************************************************************************
   Function: PutShortCutItem
   Description:
     Puts a short-cut key for a menu item. If for a command there are
     more than one key in KeySet, the first key is represented as
     a short-cut in the menu.
*/
static void PutShortCutItem(TMenuItem *pMenuItem, const TKeySet *pKeySet)
{
  const TKeySequence *pKeySeq;
  TKeySequence SearchKeySeq;
  char sKeyName[55];
  char sKeyNameBuf[55];
  int i;

  if (pMenuItem->Command == 0)
    return;

  SearchKeySeq.nCode = pMenuItem->Command;

  pKeySeq = bsearch((const void *)&SearchKeySeq,
    pKeySet->pKeySet, pKeySet->nNumberOfItems, sizeof(TKeySequence),
    CompareCmdCode);

  if (pKeySeq == NULL)
    return;  /* No short-cut key for this command */

  /*
  Check whether this is the first short-cut key for this command
  */
  if (pKeySeq == pKeySet->pKeySet)
    goto _prepare;  /* This is the first item in the array */

  while ((pKeySeq - 1)->nCode == pMenuItem->Command)  /* Stop at fist occurence */
    --pKeySeq;

_prepare:
  sKeyName[0] = '\0';
  for (i = 0; i < pKeySeq->nNumber; ++i)
  {
    disp_get_key_name(s_disp, pKeySeq->dwKeySeq[i], sKeyNameBuf, sizeof(sKeyNameBuf));
    strcat(sKeyName, sKeyNameBuf);
    ASSERT(strlen(sKeyName) < 55);
  }
  strncpy(pMenuItem->Msg2, sKeyName, MAX_MSG2 - 1);
  pMenuItem->Msg2[MAX_MSG2 - 1] = '\0';
}

/* ************************************************************************
   Function: PutShortCutKeys
   Description:
     For each item in a menu puts its short-cut key.
     Processes the Menu recursively.
*/
void _PutShortCutKeys(TMenu *pMenu, const TKeySet *pKeySet)
{
  int i;
  TMenuItem *pMenuItem;

  ASSERT(pMenu != NULL);
  ASSERT(pMenu->Orientation == MEVERTICAL || pMenu->Orientation == MEHORIZONTAL);
  ASSERT(pMenu->Items != NULL);
  ASSERT(pKeySet != NULL);
  ASSERT(pKeySet->pKeySet != NULL);
  ASSERT(pKeySet->nNumberOfItems > 0);

  for (i = 0; i < pMenu->ItemsNumber; ++i)
  {
    pMenuItem = (*(pMenu->Items))[i];
    PutShortCutItem(pMenuItem, pKeySet);
    if (pMenuItem->SubMenu != NULL)
      _PutShortCutKeys(pMenuItem->SubMenu, pKeySet);  /* Recurse to the sub-menu */
  }
}

/* ************************************************************************
   Function: PutShortCutKeys
   Description:
     _PutShortCutKeys wrap. Passes main menu as first parameters
     and current keyset as second. This function should
     be called whenever the keyset is changed (KeySet.bChanged == TRUE)
     to reflect the changes on short-cut keys representation in the menu.
*/
void PutShortCutKeys(void)
{
  _PutShortCutKeys(&meRoot, &KeySet);
}

static char cFileItems[10][MAX_MRUITEM_LEN + 1];
static TMenuItem FileItems[10];
static TMenuItem *aRecentFiles[11] =
{
  &FileItems[0], &FileItems[1],	&FileItems[2], &FileItems[3], &FileItems[4],
  &itSep,
  &FileItems[5], &FileItems[6], &FileItems[7], &FileItems[8], &FileItems[9]
};
static TMenu meRecentFiles = {MEVERTICAL, 0, (TMenuItem *(*)[])&aRecentFiles, 0};
static int nItemCount;
static int nCurFileIndex;

static char cWinItems[10][_MAX_PATH];
static TMenuItem WinItems[10];

/* ************************************************************************
   Function: PutMRUMenuItem
   Description:
     Call-back function for MRUListForEach().
     Prepares a menu entry item for the 'Recent Files' menu.
*/
static void PutMRUMenuItem(TMRUListItem *pItem, void *pContext)
{
  char sBuf[_MAX_PATH];

  if (nItemCount == 10)
    return;

  if (pItem->nCopy > 0)  /* Second copy names should be skipped */
    return;

  if (nItemCount != 9)
  {
    strcpy(cFileItems[nItemCount], "~0~ ");
    cFileItems[nItemCount][1] = nItemCount + '1';
  }
  else
    strcpy(cFileItems[9], "1~0~ ");

  PrepareFileNameTitle(pItem->sFileName, 0, sBuf, MAX_MRUITEM_LEN - 7, NULL);
  sBuf[MAX_MRUITEM_LEN - 6] = '\0';
  strcpy(&cFileItems[nItemCount][4], sBuf);
  FileItems[nItemCount].Command = cmFileOpenMRU1 + nItemCount;
  FileItems[nItemCount].func = 0;
  FileItems[nItemCount].Msg2[0] = '\0';
  FileItems[nItemCount].Prompt = (char *)&cFileItems[nItemCount];
  FileItems[nItemCount].SubMenu = 0;
  ++nItemCount;
}

/* ************************************************************************
   Function: PutFileItem
   Description:
     Call-back function for FileListForEach().
     Prepares a menu entry item for the 'Window' menu.
     TODO: CONTEXT
*/
static BOOLEAN PutFileItem(TFile *pFile, void *pContext)
{
  char sBuf[_MAX_PATH];
  char *p;

  if (nItemCount == 10)
    return TRUE;

  if (nItemCount != 9)
  {
    strcpy(sBuf, "~0~ ");
    sBuf[1] = nItemCount + '1';
  }
  else
    strcpy(sBuf, "1~0~ ");

  p = strchr(sBuf, '\0');
  PrepareFileNameTitle(pFile->sFileName, pFile->nCopy, p, MAX_WINITEM_LEN - 5, NULL);
  strcpy(cWinItems[nItemCount], sBuf);
  if (pFile->bChanged)
    strcat(cWinItems[nItemCount], " *");
  WinItems[nItemCount].Command = cmWindow1 + nItemCount;
  WinItems[nItemCount].func = 0;
  WinItems[nItemCount].Msg2[0] = '\0';
  WinItems[nItemCount].Prompt = (char *)cWinItems[nItemCount];
  WinItems[nItemCount].SubMenu = 0;

  if (nItemCount >= 5)
    aWindow[WINDOWMENUITEMS + nItemCount + 1] = &WinItems[nItemCount];
  else
    aWindow[WINDOWMENUITEMS + nItemCount] = &WinItems[nItemCount];

  if (nItemCount == 5)
    aWindow[WINDOWMENUITEMS + 5] = &itSep;

  if (pFile == GetCurrentFile())
  {
    if (nItemCount >= 5)
      nCurFileIndex = WINDOWMENUITEMS + nItemCount + 1;
    else
      nCurFileIndex = WINDOWMENUITEMS + nItemCount;
  }

  ++nItemCount;

  return TRUE;
}

static const char _sSpaces[] = "         ";  /* 9 spaces */

/* ************************************************************************
   Function: PrepareOption
   Description:
     Prepares particular value to be put in the left column of
     a vertical menu.
*/
static void PrepareOption(TMenuItem *p, BOOLEAN bFlag, int nWidth)
{
  int l;

  if (bFlag)
    strcpy(p->Msg2, sOn);
  else
    strcpy(p->Msg2, sOff);

  p->Msg2[nWidth] = '\0';
  l = strlen(p->Msg2);
  if (l < nWidth)  /* Pad right with ' ' */
    strcat(p->Msg2, &_sSpaces[l + (sizeof(_sSpaces) - nWidth - 1)]);
}

/* ************************************************************************
   Function: PrepareRecoveryTime
   Description:
     Prepares particular value to be put in the left column of
     a vertical menu.
*/
static void PrepareRecoveryTime(TMenuItem *p, int nRecoveryTime)
{
  char sBuf[10];

  sprintf(sBuf, "%dsec", nRecoveryTime);
  sprintf(p->Msg2, "%-*s", 6, sBuf);
}

/* ************************************************************************
   Function: PrepareRightMargin
   Description:
     Prepares particular value to be put in the left column of
     a vertical menu.
*/
static void PrepareRightMargin(TMenuItem *p, int nRightMargin, int nWidth)
{
  sprintf(p->Msg2, "%-*d", nWidth, nRightMargin);
}

/* ************************************************************************
   Function: PrepareFileSaveMode
   Description:
     Displays the default file output mode.
*/
static void PrepareFileSaveMode(TMenuItem *p, int nRightMargin, int nWidth)
{
  ASSERT(nFileSaveMode >= -1);
  ASSERT(nFileSaveMode <= CRtype);

  sprintf(p->Msg2, "%-*s", nWidth, sEOLTypes[nFileSaveMode + 1]);
}

/* ************************************************************************
   Function: PrepareEditorOptions
   Description:
     Prepares all the values (on/off, etc..) of the options
     in the Options|Editor_Options submenu.
*/
static void PrepareEditorOptions(void)
{
  PrepareOption(&itOptionsBackup, bBackup, 6);
  PrepareOption(&itOptionsLastFiles, bRestoreLastFiles, 6);
  PrepareOption(&itOptionsPersistentBlocks, bPersistentBlocks, 6);
  PrepareOption(&itOptionsOverwriteBlocks, bOverwriteBlocks, 6);
  PrepareOption(&itOptionsSyntax, bSyntaxHighlighting, 6);
  PrepareOption(&itOptionsMSyntx, bMatchPairHighlighting, 6);
  PrepareOption(&itOptionsHighlightIF0, bIF0Highlighting, 6);
  PrepareOption(&itOptionsAscendingSort, bAscendingSort, 6);
  PrepareOption(&itOptionsCaseSensitiveSort, bCaseSensitiveSort, 6);
  PrepareRecoveryTime(&itOptionsRecoveryTime, nRecoveryTime);
  PrepareRightMargin(&itOptionsRMarg, nRightMargin, 6);
  PrepareFileSaveMode(&itOptionsSaveAs, nFileSaveMode, 6);
}

static BOOLEAN bOptionsChanged;  /* Counts for a single menu session */

/* ************************************************************************
   Function: ToggleBackup
   Description:
     This is a call-back function called form the main menu.
*/
static void ToggleBackup(void)
{
  bOptionsChanged = TRUE;

  bBackup = !bBackup;
  PrepareOption(&itOptionsBackup, bBackup, 6);
}

/* ************************************************************************
   Function: ToggleLastFiles
   Description:
     This is a call-back function called form the main menu.
*/
static void ToggleLastFiles(void)
{
  bOptionsChanged = TRUE;

  bRestoreLastFiles = !bRestoreLastFiles;
  PrepareOption(&itOptionsLastFiles, bRestoreLastFiles, 6);
}

/* ************************************************************************
   Function: TogglePersistentBlocks
   Description:
     This is a call-back function called form the main menu.
*/
static void TogglePersistentBlocks(void)
{
  bOptionsChanged = TRUE;

  bPersistentBlocks = !bPersistentBlocks;
  PrepareOption(&itOptionsPersistentBlocks, bPersistentBlocks, 6);
}

/* ************************************************************************
   Function: ToggleOverwriteBlocks
   Description:
     This is a call-back function called form the main menu.
*/
static void ToggleOverwriteBlocks(void)
{
  bOptionsChanged = TRUE;

  bOverwriteBlocks = !bOverwriteBlocks;
  PrepareOption(&itOptionsOverwriteBlocks, bOverwriteBlocks, 6);
}

/* ************************************************************************
   Function: ToggleSyntax
   Description:
     This is a call-back function called form the main menu.
*/
static void ToggleSyntax(void)
{
  bOptionsChanged = TRUE;

  bSyntaxHighlighting = !bSyntaxHighlighting;
  PrepareOption(&itOptionsSyntax, bSyntaxHighlighting, 6);
}

/* ************************************************************************
   Function: ToggleMSyntx
   Description:
     This is a call-back function called form the main menu.
*/
static void ToggleMSyntx(void)
{
  bOptionsChanged = TRUE;

  bMatchPairHighlighting = !bMatchPairHighlighting;
  PrepareOption(&itOptionsMSyntx, bMatchPairHighlighting, 6);
}

/* ************************************************************************
   Function: ToggleIF0
   Description:
     This is a call-back function called form the main menu.
*/
static void ToggleIF0(void)
{
  bOptionsChanged = TRUE;

  bIF0Highlighting = !bIF0Highlighting;
  PrepareOption(&itOptionsHighlightIF0, bIF0Highlighting, 6);
}

/* ************************************************************************
   Function: ToggleAscendingSort
   Description:
     This is a call-back function called form the main menu.
*/
void ToggleAscendingSort(void)
{
  bOptionsChanged = TRUE;

  bAscendingSort = !bAscendingSort;
  PrepareOption(&itOptionsAscendingSort, bAscendingSort, 6);
}

/* ************************************************************************
   Function: ToggleCaseSensitiveSort
   Description:
     This is a call-back function called form the main menu.
*/
void ToggleCaseSensitiveSort(void)
{
  bOptionsChanged = TRUE;

  bCaseSensitiveSort = !bCaseSensitiveSort;
  PrepareOption(&itOptionsCaseSensitiveSort, bCaseSensitiveSort, 6);
}

#define DisplayError(sMsg) \
  ConsoleMessageProc(s_disp, NULL, MSG_ERROR | MSG_OK, NULL, sMsg)

/* ************************************************************************
   Function: InputRecoveryTime
   Description:
     This is a call-back function called form the main menu
     to change the current recovery time value.
*/
static void InputRecoveryTime(void)
{
  char sRecoveryTime[10];
  disp_char_buf_t *pStatLn;
  int nVal;

  pStatLn = SaveStatusLine(s_disp);
  sprintf(sRecoveryTime, "%d", nRecoveryTime);
  /* was itoa(nRecoveryTime, sRecoveryTime, 10); */
  if (EnterLn(sInputRecoveryTime, sRecoveryTime, 10, NULL, NULL, NULL, NULL, TRUE, s_disp))
  {
    if (!ValStr(sRecoveryTime, &nVal, 10))
      DisplayError(sInvalidRecoveryTimeValue);
    else
    {
      if (nVal < 0 || nVal >= MAX_RECOVERY_TIME_VAL)
        DisplayError(sInvalidRecoveryTimeRange);
      else
      {
        nRecoveryTime = nVal;
        PrepareRecoveryTime(&itOptionsRecoveryTime, nRecoveryTime);
        bOptionsChanged = TRUE;
      }
    }
  }
  RestoreStatusLine(pStatLn, s_disp);
}

/* ************************************************************************
   Function: InputRightMargin
   Description:
     This is a call-back function called form the main menu
     to change the current right margin value.
*/
static void InputRightMargin(void)
{
  char sRightMargin[10];
  disp_char_buf_t *pStatLn;
  int nVal;

  pStatLn = SaveStatusLine(s_disp);
  sprintf(sRightMargin, "%d", nRightMargin);
  /* was itoa(nRightMargin, sRightMargin, 10); */
  if (EnterLn(sInputRightMargin, sRightMargin, 10, NULL, NULL, NULL, NULL, TRUE, s_disp))
  {
    if (!ValStr(sRightMargin, &nVal, 10))
      DisplayError(sInvalidRightMarginValue);
    else
    {
      if (nVal < 0 || nVal >= MAX_RIGHT_MARGIN_VAL)
        DisplayError(sInvalidRightMarginRange);
      else
      {
        nRightMargin = nVal;
	PrepareRightMargin(&itOptionsRMarg, nRightMargin, 6);
        bOptionsChanged = TRUE;
      }
    }
  }
  RestoreStatusLine(pStatLn, s_disp);
}

/* ************************************************************************
   Function: InputFileSaveMode
   Description:
     This is a call-back function called form the main menu
     to change the current file save mode.
*/
static void InputFileSaveMode(void)
{
  const char *pTypesMenu[10];
  const char **pMenuItem;
  char **pTypes;
  char sResult[35];  /* I hope it will fit */
  int nType;

  pMenuItem = pTypesMenu;
  for (pTypes = sEOLTypesDetailed; pTypes - sEOLTypesDetailed < _countof(sEOLTypesDetailed); ++pTypes)
    *pMenuItem++ = *pTypes;

  *pMenuItem = NULL;  /* Point the end menu index */

  nType = nFileSaveMode + 1;
  if (UMenu(SelectedX + 6, SelectedY + 1, 1, 6, 13,
      "modes", pTypesMenu, sResult, &nType, NULL, NULL, coUMenu, s_disp))
  {
    nFileSaveMode = nType - 1;
    PrepareFileSaveMode(&itOptionsSaveAs, nFileSaveMode, 6);
    bOptionsChanged = TRUE;
  }
}

/***************************************************
Following are the funtions to maintain File Type Set
***************************************************/

static const char *pMenu[MAX_DOCS];
static int nCurItem = 0;
static TDocType *pCurItem;
static TDocType EditItem;

/* ************************************************************************
   Function: PrepareFileExtAndType
   Description:
     Puts in the menu the file extention and type.
     Source is EditItem.
     Destination is itFExt (item in a menu).
*/
static void PrepareFileExtAndType(void)
{
  char sBuf[10];
  int l;
  char *sType;

  ASSERT(MAX_MSG2 > 10);

  /* Prepare file .ext */
  strncpy(sBuf, EditItem.sExt, 9);
  sBuf[9] = '\0';
  l = strlen(sBuf);
  if (l < 8)  /* Pad right with ' ' */
    strcat(sBuf, &_sSpaces[l]);
  strcpy(itFExt.Msg2, sBuf);

  /* Prepare file type */
  sType = GetSyntaxTypeName(EditItem.nType);
  ASSERT(sType != NULL);
  strncpy(sBuf, sType, 9);
  sBuf[9] = '\0';
  l = strlen(sBuf);
  if (l < 8)  /* Pad right with ' ' */
    strcat(sBuf, &_sSpaces[l]);
  strcpy(itFType.Msg2, sBuf);
}

/* ************************************************************************
   Function: PrepareTab
   Description:
     Prepares particular value to be put in the left column of
     a vertical menu.
*/
static void PrepareTab(TMenuItem *p, int nTabSize, int nWidth)
{
  sprintf(p->Msg2, "%-*d", nWidth, nTabSize);
}

/* ************************************************************************
   Function: InputFileExtention
   Description:
     This is a call-back function called form the document type
     editing menu.
*/
static void InputFileExtention(void)
{
  char sFileExt[MAX_EXT_LEN];
  disp_char_buf_t *pStatLn;

  pStatLn = SaveStatusLine(s_disp);
  strcpy(sFileExt, EditItem.sExt);

  /* MAX_EXT_LEN - 1 to put '.' in case user missed to enter it */
  if (EnterLn(sInputFileExt, sFileExt, MAX_EXT_LEN - 1, NULL, NULL, NULL, NULL, TRUE, s_disp))
  {
    if (sFileExt[0] != '\0')
    {
      strcpy(EditItem.sExt, sFileExt);
      PrepareFileExtAndType();
      bOptionsChanged = TRUE;
    }
  }
  RestoreStatusLine(pStatLn, s_disp);
}

/* ************************************************************************
   Function: InputFileType
   Description:
     This is a call-back function called form the document type
     editing menu.
*/
static void InputFileType(void)
{
  const char *pTypesMenu[10];
  const char **pMenuItem;
  char *pTypes;
  char sResult[35];  /* I hope it will fit */
  int nType;

  pMenuItem = pTypesMenu;
  nType = 0;
  do
  {
    pTypes = GetSyntaxTypeName(nType++);
    if (pTypes != NULL)
      *pMenuItem++ = pTypes;
  }
  while (pTypes != NULL);

  *pMenuItem = NULL;  /* Point the end menu index */

  nType = EditItem.nType;
  if (UMenu(SelectedX + 1, SelectedY + 1, 1, 6, 12,
      "types", pTypesMenu, sResult, &nType, NULL, NULL, coUMenu, s_disp))
  {
    EditItem.nType = nType;
    PrepareFileExtAndType();
    bOptionsChanged = TRUE;
  }
}

/* ************************************************************************
   Function: ToggleAutoIndent
   Description:
     This is a call-back function called form the document type
     editing menu.
*/
static void ToggleAutoIndent(void)
{
  bOptionsChanged = TRUE;

  EditItem.bAutoIndent = !EditItem.bAutoIndent;
  PrepareOption(&itAutoInd, EditItem.bAutoIndent, 9);
}

/* ************************************************************************
   Function: ToggleUseTab
   Description:
     This is a call-back function called form the document type
     editing menu.
*/
static void ToggleUseTab(void)
{
  bOptionsChanged = TRUE;

  EditItem.bUseTabs = !EditItem.bUseTabs;
  PrepareOption(&itUseTab, EditItem.bUseTabs, 9);
}

/* ************************************************************************
   Function: ToggleOptFill
   Description:
     This is a call-back function called form the document type
     editing menu.
*/
static void ToggleOptFill(void)
{
  bOptionsChanged = TRUE;

  EditItem.bOptimalFill = !EditItem.bOptimalFill;
  PrepareOption(&itOptFill, EditItem.bOptimalFill, 9);
}

/* ************************************************************************
   Function: ToggleBUnind
   Description:
     This is a call-back function called form the document type
     editing menu.
*/
static void ToggleBUnind(void)
{
  bOptionsChanged = TRUE;

  EditItem.bBackspaceUnindent = !EditItem.bBackspaceUnindent;
  PrepareOption(&itBUnind, EditItem.bBackspaceUnindent, 9);
}

/* ************************************************************************
   Function: ToggleCTabs
   Description:
     This is a call-back function called form the document type
     editing menu.
*/
static void ToggleCTabs(void)
{
  bOptionsChanged = TRUE;

  EditItem.bCursorThroughTabs = !EditItem.bCursorThroughTabs;
  PrepareOption(&itCTabs, EditItem.bCursorThroughTabs, 9);
}

#define DISPLAY_ERR_MSG(s) \
    ConsoleMessageProc(s_disp, NULL, MSG_ERROR | MSG_OK, NULL, s)

/* ************************************************************************
   Function: InputTabSize
   Description:
     This is a call-back function called form the document type
     editing menu.
*/
static void InputTabSize(void)
{
  char sTabSize[10];
  disp_char_buf_t *pStatLn;
  int nVal;

  pStatLn = SaveStatusLine(s_disp);
  sprintf(sTabSize, "%d", EditItem.nTabSize);
  /* was itoa(EditItem.nTabSize, sTabSize, 10); */
  if (EnterLn(sInputTabSize, sTabSize, 10, NULL, NULL, NULL, NULL, TRUE, s_disp))
  {
    if (!ValStr(sTabSize, &nVal, 10))
      DISPLAY_ERR_MSG(sInvalidTabValue);
    else
    {
      if (nVal < 0 || nVal >= MAX_TAB_SIZE)
        DISPLAY_ERR_MSG(sInvalidTabRange);
      else
      {
        EditItem.nTabSize = nVal;
        PrepareTab(&itTabSz, EditItem.nTabSize, 9);
        bOptionsChanged = TRUE;
      }
    }
  }
  RestoreStatusLine(pStatLn, s_disp);
}

/* ************************************************************************
   Function: ToggleWWrap
   Description:
     This is a call-back function called form the document type
     editing menu.
*/
static void ToggleWWrap(void)
{
  bOptionsChanged = TRUE;

  EditItem.bWordWrap = !EditItem.bWordWrap;
  PrepareOption(&itWWrap, EditItem.bWordWrap, 9);
}

/* ************************************************************************
   Function: PrepareDocEditMenu
   Description:
     Prepares meDocumentOpts menu acording to information
     in	EditItem
*/
static void PrepareDocEditMenu(BOOLEAN bAllowCancel)
{
  PrepareFileExtAndType();
  PrepareOption(&itAutoInd, EditItem.bAutoIndent, 9);
  PrepareOption(&itUseTab, EditItem.bUseTabs, 9);
  PrepareOption(&itOptFill, EditItem.bOptimalFill, 9);
  PrepareOption(&itBUnind, EditItem.bBackspaceUnindent, 9);
  PrepareOption(&itCTabs, EditItem.bCursorThroughTabs, 9);
  PrepareOption(&itWWrap, EditItem.bWordWrap, 9);
  PrepareTab(&itTabSz, EditItem.nTabSize, 9);
  meDocumentOpts.ItemsNumber = _countof(aDocumentOpts);
  if (!bAllowCancel)  /* remove these items */
    meDocumentOpts.ItemsNumber = _countof(aDocumentOpts) - 3;
}

/* ************************************************************************
   Function: MakeDocMenuIndex
   Description:
     Prepares an index array containing all the document type set
     items to be used by UMenu().
*/
static void MakeDocMenuIndex(void)
{
  const char **pMenuItem;
  TDocType *pDocumentItem;

  pMenuItem = pMenu;
  for (pDocumentItem = DocumentTypes; !IS_END_OF_DOC_LIST(pDocumentItem); ++pDocumentItem)
    *pMenuItem++ = pDocumentItem->sExt;

  *pMenuItem = NULL;  /* Point the end menu index */
}

/* ************************************************************************
   Function: CountDocType
   Description:
     Counts the current document type set.
     Finds the correspondent item for the currently selected
     item in UMenu().
*/
static void CountDocType(int nCurItem, int *pnItems, TDocType **ppCurrentItem)
{
  TDocType *pDocumentItem;

  ASSERT(nCurItem >= 0);
  ASSERT(pnItems != NULL);
  ASSERT(ppCurrentItem != NULL);

  *pnItems = -1;
  *ppCurrentItem = NULL;
  for (pDocumentItem = DocumentTypes; !IS_END_OF_DOC_LIST(pDocumentItem); ++pDocumentItem)
  {
    if (pDocumentItem - DocumentTypes == nCurItem)
      *ppCurrentItem = pDocumentItem;
  }
  *pnItems = (int)(pDocumentItem - DocumentTypes);
}

/* ************************************************************************
   Function: DocTypeEdit_DlgProc
   Description:
     This functios is called to execute
     a specific	command by UMenu().
*/
static int DocTypeEdit_DlgProc(int nCurItem, int nCmd)
{
  int nNumberOfItems;

  ASSERT(nCurItem >= 0);
  ASSERT(nCmd >= 1 && nCmd <= 3);

  CountDocType(nCurItem, &nNumberOfItems, &pCurItem);
  memcpy(&EditItem, pCurItem, sizeof(TDocType));

  switch (nCmd)
  {
    case 1:  /* Insert new document type item */
      if (DocTypesEmptyEntries() == 0)
      {
        DISPLAY_ERR_MSG(sNoMoreTypes);
	break;
      }
      memcpy(&EditItem, &DefaultDocOptions, sizeof(TDocType));
      PrepareDocEditMenu(TRUE);
      meDocumentOpts.Sel = 0;  /* Always put selection bar at first item */
      if (Menu(&meDocumentOpts, UMenu_SelectedX - 2, UMenu_SelectedY + 1, 0, 0) == 1)
      {
        /* Insert EditItem before pCurItem */
	memmove(pCurItem + 1, pCurItem,
	  (nNumberOfItems - (int)(pCurItem - DocumentTypes) + 1) * sizeof(TDocType));
	memcpy(pCurItem, &EditItem, sizeof(TDocType));
        MakeDocMenuIndex();
      }
      break;
    case 2:  /* Delete the current document type item */
      memcpy(pCurItem, pCurItem + 1, (nNumberOfItems + 1) * sizeof(TDocType));
      MakeDocMenuIndex();
      break;
    case 3:  /* Edit the current document type item */
      PrepareDocEditMenu(FALSE);
      meDocumentOpts.Sel = 0;  /* Always put selection bar at first item */
      Menu(&meDocumentOpts, UMenu_SelectedX - 2, UMenu_SelectedY + 1, 0, 0);
      memcpy(pCurItem, &EditItem, sizeof(TDocType));  /* exit with 'OK' */
      break;
  }
  return 0;
}

static TKeySequence DocTypeEdit[] =
{
  {1, DEF_KEY1(KEY(0, kbIns))},
  {2, DEF_KEY1(KEY(0, kbDel))},
  {3, DEF_KEY1(KEY(0, kbEnter))},
  {END_OF_KEY_LIST_CODE}  /* LastItem */
};

/* ************************************************************************
   Function: EditDocumentTypeSet
   Description:
     Edits the document type set for the current editor session.
     Usually invoked from Options|Document Type	Set|Edit document type set.
     Editing is managed from UMenu() called with a specific
     key map to execute the document set editing function such
     as Insert, Remove and Edit over the selected document type item.
*/
static void EditDocumentTypeSet(void)
{
  char sResult[MAX_EXT_LEN];
  disp_char_buf_t *pStatLn;

  MakeDocMenuIndex();

  pStatLn = SaveStatusLine(s_disp);
  DisplayStatusStr2(sStatDocEdit, coStatusTxt, coStatusShortCut, s_disp);
  UMenu(SelectedX + 1, SelectedY + 1, 1, 8, 20,
        "docs", pMenu, sResult, &nCurItem,
        DocTypeEdit, DocTypeEdit_DlgProc, coUMenu, s_disp);
  RestoreStatusLine(pStatLn, s_disp);

  /*
  Make the changes valid for the file being currently edited
  */
  if (bOptionsChanged)
    ActivateDocumentOptions();
}

/* ************************************************************************
   Function: EditDefaultDocument
   Description:
     Edits the options valid for all the documents with
     extentions not in DocumentSet list.
     Usially invoked from Options|Document Type Set|Default document options.
*/
static void EditDefaultDocument(void)
{
  memcpy(&EditItem, &DefaultDocOptions, sizeof(TDocType));
  PrepareDocEditMenu(FALSE);
  meDocumentOpts.Sel = 0;  /* Always put selection bar at first item */

  /* Diasable items that will have no meaning */
  itFExt.Options |= meDISABLED;
  itFType.Options |= meDISABLED;

  Menu(&meDocumentOpts, SelectedX - 2, SelectedY + 1, 0, 0);
  memcpy(&DefaultDocOptions, &EditItem, sizeof(TDocType));  /* exit with 'OK' */

  /* Enable back */
  itFExt.Options &= ~meDISABLED;
  itFType.Options &= ~meDISABLED;

  /*
  Make the changes valid for the file being currently edited
  */
  if (bOptionsChanged)
    ActivateDocumentOptions();
}

/* ************************************************************************
   Function: EdSetDefaultDocumentTypeSet
   Description:
*/
static void SetDefaultDocumentTypeSet(void)
{
  bOptionsChanged = TRUE;

  CopyDocumentTypeSet(DocumentTypes, DefaultDocuments);
  memcpy(&DefaultDocOptions, &_DefaultDocOptions, sizeof(TDocType));
}


/*
Static variables to pass parematers to EditDefaultDocumentTypeSet()
call-back function.
*/
static TDocType *pEditDoc;

/* ************************************************************************
   Function: EditCurrentDocumentTypeSet
   Description:
*/
static void EditCurrentDocumentTypeSet(void)
{
  memcpy(&EditItem, pEditDoc, sizeof(TDocType));
  PrepareDocEditMenu(FALSE);
  meDocumentOpts.Sel = 0;  /* Always put selection bar at first item */

  Menu(&meDocumentOpts, SelectedX - 2, SelectedY + 1, 0, 0);
  memcpy(pEditDoc, &EditItem, sizeof(TDocType));  /* exit with 'OK' */

  /*
  Make the changes valid for the file being currently edited
  */
  if (bOptionsChanged)
    ActivateDocumentOptions();
}

/***************************************************
End of File Type Set functions
***************************************************/

/* ************************************************************************
   Function: CompareInt
   Description:
     Call-back function to be passed to qsort and bsearch.
*/
static int CompareCmdID(const void *arg1, const void *arg2)
{
  const TCmdDesc *i1 = (const TCmdDesc *)arg1;
  const TCmdDesc *i2 = (const TCmdDesc *)arg2;

  ASSERT(i1->nCode > 0);
  ASSERT(i2->nCode > 0);
  ASSERT(i1->nCode < MAX_CMD_CODE);
  ASSERT(i2->nCode < MAX_CMD_CODE);

  return (i1->nCode - i2->nCode);
}

/* ************************************************************************
   Function: InvokeCommandHelp
   Description:
     Shows context help for specific command code.
*/
void InvokeCommandHelp(int nCmdCode, int x, int y)
{
  const TCmdDesc *pCmdItem;
  TCmdDesc CmdItemKey;
  extern int nNumberOfCommands;  /* prepared in main2.c */
  const char *psHelpPage;
  const char *psMenuItem;

  if (nCmdCode <= 0)
    goto _no_help;

  ASSERT(nNumberOfCommands > 0);

  CmdItemKey.nCode = nCmdCode;
  pCmdItem = bsearch(&CmdItemKey, Commands, nNumberOfCommands,
    sizeof(TCmdDesc), CompareCmdID);
  if (pCmdItem == NULL)
  {
_no_help:
    psHelpPage = sHelpPageNone;
    psMenuItem = sHelpNoHelpProvided;
  }
  else
    if (pCmdItem->sHelpPage == NULL)
    {
      psHelpPage = sHelpPageNone;
      psMenuItem = sHelpNoPointer;
    }
    else
    {
      psHelpPage = pCmdItem->sHelpPage;
      psMenuItem = pCmdItem->sCtxMenu;
    }
  HelpPushContext(&stCtxHelp, psHelpPage, psMenuItem, NULL);

  disp_cursor_goto_xy(s_disp, x, y);
  ShowContextHelp(&stCtxHelp);
  HelpPopContext(&stCtxHelp, 1);
}

/* ************************************************************************
   Function: InvokeHelp
   Description:
     Call-back function. Called whenever in a pull-down menu the help key F1
     is pressed. Prepares entries in the context help stack and
     invokes the context help procedure.

     Help is shown either for nCmdCode or for nHelpCode.
     If nCmdCode is 0 then nHelpCode is used to extract the
     help page title to be shown.
*/
static void InvokeHelp(int nCmdCode, const char *psPage,
  const char *psCtxHlpItem, const char *psInfoFile)
{
  if (nCmdCode == 0 && psPage != NULL)
  {
    HelpPushContext(&stCtxHelp, pCtxSep, pCtxSep, NULL);
    HelpPushContext(&stCtxHelp, "pulldown_menu", "Help on pull down menu navigation", NULL);
    HelpPushContext(&stCtxHelp, psPage, psCtxHlpItem, psInfoFile);
    /* SelectedX & SelectedY are global variables showing current menu item pos */
    disp_cursor_goto_xy(s_disp, SelectedX + 4, SelectedY + 2);
    ShowContextHelp(&stCtxHelp);
    HelpPopContext(&stCtxHelp, 3);
  }
  else
  {
    HelpPushContext(&stCtxHelp, pCtxSep, pCtxSep, NULL);
    HelpPushContext(&stCtxHelp, "pulldown_menu", "Help on pull down menu navigation", NULL);
    InvokeCommandHelp(nCmdCode, SelectedX + 4, SelectedY + 2);
    HelpPopContext(&stCtxHelp, 2);
  }
}

/* ************************************************************************
   Function: ExecuteMenu
   Description:
     dwKey is the key pressed. If this key is a Alt+key combination
     or Esc, then the root menu is selected.
   Returns:
     -2 -- menu was not activated
*/
int ExecuteMenu(DWORD dwKey, dispc_t *_disp)
{
  int nMenuCode;
  char sItemEditDocOpt[MAX_EXT_LEN + 50];

  s_disp = _disp;

  bOptionsChanged = FALSE;

  if (dwKey != KEY(0, kbEsc))
    if (SH_STATE(dwKey) != kbAlt)
      return -2;

  /*
  Put MRU files in 'Recent Files' submenu.
  */
  nItemCount = 0;
  MRUListForEach(pMRUFilesList, PutMRUMenuItem, TRUE, NULL);
  meRecentFiles.ItemsNumber = nItemCount;
  if (nItemCount > 5)
    ++meRecentFiles.ItemsNumber;  /* Include the separator */
  itFileRecentFiles.SubMenu = &meRecentFiles;

  /*
  Put all files in memory in 'Window' submenu.
  */
  nItemCount = 0;
  nCurFileIndex = -1;
  FileListForEach(pFilesInMemoryList, PutFileItem, bConsequtiveWinFiles, NULL);
  meWindow.ItemsNumber = nItemCount + WINDOWMENUITEMS;
  if (nItemCount > 5)
    ++meWindow.ItemsNumber;  /* Include the separator */
  /*
  Put the selection marker at the position of the current file
  */
  if (nCurFileIndex != -1)
    meWindow.Sel = nCurFileIndex;

  /*
  Prepare 'Edit options' submenu
  */
  PrepareEditorOptions();

  /*
  Prepare 'Edit default document set' item.
  */
  pEditDoc = DetectDocument(DocumentTypes, GetCurrentFile()->sFileName);
  if (pEditDoc != NULL)
  {
    sprintf(sItemEditDocOpt, "~.~Options for %s ...", pEditDoc->sExt);
    itOptionsDocumentTypes4.func = EditCurrentDocumentTypeSet;
  }
  else  /* Propose options for default document type */
  {
    strcpy(sItemEditDocOpt, "~.~Options for plain document...");
    itOptionsDocumentTypes4.func = EditDefaultDocument;
  }
  itOptionsDocumentTypes4.Prompt = sItemEditDocOpt;

  /*
  Invoke the menu function to navigate the menu tree.
  */
  meRoot.disp = s_disp;
  if (SH_STATE(dwKey) == kbAlt)  /* Invoke with Alt combination for the root menu */
    nMenuCode = Menu(&meRoot, 0, 0, disp_wnd_get_width(s_disp), dwKey);
  else
    nMenuCode = Menu(&meRoot, 0, 0, disp_wnd_get_width(s_disp), 0);

  /* TODO: disp, This flag should be exported to the workspace structure*/
  //bCtrlReleased = TRUE; /* Validate this flag */

  if (nMenuCode == cmSetDefaultDoc)
  {
    SetDefaultDocumentTypeSet();
    nMenuCode = -1;
  }

  GetCurrentFile()->bUpdatePage = TRUE;

  if (bOptionsChanged)
    ActivateDocumentOptions();

  if (nMenuCode == 0)
    nMenuCode = cmNotReady;

  if (nMenuCode == cmNotReady)
  {
    CmdNotReady(NULL);
    nMenuCode = -1;
  }

  return nMenuCode;
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

