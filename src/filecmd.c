/*

File: filecmd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 22nd December, 1998
Descrition:
  File commands.

*/

#include "global.h"
#include "filemenu.h"
#include "file2.h"
#include "wline.h"
#include "l2disp.h"
#include "filemenu.h"
#include "wrkspace.h"
#include "l1def.h"
#include "defs.h"
#include "main2.h"
#include "nav.h"
#include "mru.h"
#include "undo.h"
#include "bookm.h"
#include "cmdc.h"
#include "filecmd.h"
#include "memory.h"
#include "disp.h"
#include "palette.h"
#include "search.h"
#include "contain.h"
#include "diag.h"

/* ************************************************************************
   Function: CheckToRemoveRecoveryFile
   Description:
     If all the actions of a file are undone (bChanged = FALSE)
     the existing recovery file should be removed.
*/
static void CheckToRemoveRecoveryFile(TFile *pFile, dispc_t *disp)
{
  ASSERT(VALID_PFILE(pFile));

  if (pFile->bChanged)
    return;

  /*
  Remove the recovery file.
  */
  if (FileExists(pFile->sRecoveryFileName))
  {
    /* .rec file exists. Remove the .rec file */
    if (unlink(pFile->sRecoveryFileName) != 0)
    {
      /* unlink failed: display error */
      ConsoleMessageProc(disp, NULL, MSG_ERRNO | MSG_ERROR | MSG_OK,
        pFile->sRecoveryFileName, NULL);
    }
  }
}

/* ************************************************************************
   Function: CheckToRemoveBlankFile
   Description:
     The initial blank file is to be automaticly closed.
     This function checks for the conditions and returns
     TRUE to indicate that "noname" is to be closed or FALSE.
*/
BOOLEAN CheckToRemoveBlankFile(void)
{
  TFile *pTopFile;
  char sExpandedNoname[_MAX_PATH];

  /*
  If there is only one file and the file is new and the file is noname
  and the file is empty -- close the file, this is the initial blank file
  */
  if (GetNumberOfFiles() == 1)
  {
    pTopFile = GetFileListTop(pFilesInMemoryList);
    strcpy(sExpandedNoname, sNoname);
    GetFullPath(sExpandedNoname);
    if (pTopFile->bNew && filestrcmp(pTopFile->sFileName, sExpandedNoname) == 0 &&
      !pTopFile->bChanged && pTopFile->nNumberOfLines == 0)
    {
//      RemoveMRUItem(pMRUFilesList, pTopFile->sFileName, 0);
//      CheckToRemoveRecoveryFile(pTopFile);  /* If everything is undone */
//      RemoveTopFile(pFilesInMemoryList, pMRUFilesList);
      return TRUE;
    }
  }

  return FALSE;
}

/* ************************************************************************
   Function: RemoveBlankFile
   Description:
     To be called after a file is loaded
     in order to remove "noname" file.
*/
void RemoveBlankFile(BOOLEAN bRemoveFlag, dispc_t *disp)
{
  TFile *pTopFile;

  if (!bRemoveFlag)
    return;
  if (GetNumberOfFiles() != 2)  /* Should be the new file + "noname" */
    return;
  SetTopFileByZOrder(pFilesInMemoryList, 1);
  pTopFile = GetFileListTop(pFilesInMemoryList);
  RemoveMRUItem(pMRUFilesList, pTopFile->sFileName, 0);
  CheckToRemoveRecoveryFile(pTopFile, disp);  /* If everything is undone */
  RemoveTopFile(pFilesInMemoryList, pMRUFilesList, disp);
}

/* ************************************************************************
   Function: CmdFileNew
   Description:
*/
void CmdFileNew(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));
  AddNewFile(pFilesInMemoryList, disp);
}

/* ************************************************************************
   Function: CmdFileOpen
   Description:
*/
void CmdFileOpen(void *pCtx)
{
  static char sOpenFileName[_MAX_PATH] = {'\0'};
  BOOLEAN bRemoveFlag;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));
  if (!InputFileName(sPromptOpenFile, sOpenFileName, pFileOpenHistory, sAllMask, disp))
    return;

  bRemoveFlag = CheckToRemoveBlankFile();

  LoadFile(sOpenFileName, pFilesInMemoryList, pMRUFilesList,
    DocumentTypes, FALSE, FALSE, disp);

  RemoveBlankFile(bRemoveFlag, disp);
}

/* ************************************************************************
   Function: CmdFileOpenAsReadOnly
   Description:
*/
void CmdFileOpenAsReadOnly(void *pCtx)
{
  static char sOpenFileName[_MAX_PATH] = {'\0'};
  BOOLEAN bRemove;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));
  if (!InputFileName(sPromptOpenFile, sOpenFileName, pFileOpenHistory, sAllMask, disp))
    return;

  bRemove = CheckToRemoveBlankFile();

  LoadFile(sOpenFileName, pFilesInMemoryList, pMRUFilesList, DocumentTypes,
    FALSE, TRUE, disp);

  RemoveBlankFile(bRemove, disp);
}

/* ************************************************************************
   Function: CmdFileSave
   Description:
*/
void CmdFileSave(void *pCtx)
{
  TFile *pFile;
  char sFileName[_MAX_PATH];
  char sFilePath[_MAX_PATH];
  dispc_t *disp;

  pFile = CMDC_PFILE(pCtx);
  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  FSplit(pFile->sFileName, sFilePath, sFileName, "*", TRUE, TRUE);
  if (pFile->bNew && filestrcmp(sFileName, sNoname) == 0)
    CmdFileSaveAs(pFile);
  else
    StoreFile(pFile, pMRUFilesList, disp);

  pFile->bUpdateStatus = TRUE;
}

/* ************************************************************************
   Function: CmdFileSaveAs
   Description:
*/
void CmdFileSaveAs(void *pCtx)
{
  static char sFileName[_MAX_PATH] = {'\0'};
  char sExtendedName[_MAX_PATH];
  void OnFileSwitched(dispc_t *disp, TFile *pLastFile);
  TFile *pFile;
  dispc_t *disp;

  pFile = CMDC_PFILE(pCtx);
  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (!InputFileName(sPromptSaveAs, sFileName, pFileOpenHistory, sAllMask, disp))
    return;

  strcpy(sExtendedName, sFileName);
  GetFullPath(sExtendedName);
  if (SearchFileList(pFilesInMemoryList, sExtendedName, 0) != -1)
  {
    ConsoleMessageProc(disp, NULL, MSG_ERROR, sFileName, sFileExistsInMemory);
    return;
  }

  if (FileExists(sExtendedName))
  {
    if (ConsoleMessageProc(disp, NULL, MSG_WARNING | MSG_YESNO, sFileName, sFileExists) != 0)
      return;
  }

  StoreFileAs(sExtendedName,
    pFile, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);

  pFile->bUpdateStatus = TRUE;
  OnFileSwitched(disp, pFile);
}

typedef struct _CheckSaveContext
{
  BOOLEAN bCanceled;
  BOOLEAN bCheck;
  wrkspace_data_t *wrkspace;
} TCheckSaveContext;

/* ************************************************************************
   Function: CheckSave
   Description:
     A call-back function invoked to check wether the file is changed
     and to ask user to save the file to disk.
*/
static BOOLEAN CheckSave(TFile *pFile, void *_pContext)
{
  TCheckSaveContext *pContext;
  int nFileNumber;
  dispc_t *disp;
  cmdc_t cmdc;

  ASSERT(VALID_PFILE(pFile));

  pContext = _pContext;
  disp = wrkspace_get_disp(CMDC_WRKSPACE(pContext));
  if (pFile->bChanged)
  {
    if (!pContext->bCheck)
      goto _saveafile;

    switch (ConsoleMessageProc(disp, NULL, MSG_WARNING | MSG_YESNOCANCEL,
      pFile->sFileName, sAskSaveName))
    {
      case 0:  /* Yes or Enter */
_saveafile:
        nFileNumber = SearchFileList(pFilesInMemoryList, pFile->sFileName, 0);
        ASSERT(nFileNumber >= 0);
        SetTopFileByLoadNumber(pFilesInMemoryList, nFileNumber);
        /* TODO: cmdc_t! */
        CMDC_SET(cmdc, pContext->wrkspace, pFile);
        CmdFileSave(&cmdc);
        break;
      case 1:  /* No */
        return TRUE;
      case 2:  /* Esc/Cancel */
        pContext->bCanceled = TRUE;
        return FALSE;  /* Don't close the file */
      default:
        ASSERT(0);
    }
  }
  return TRUE;
}

/* ************************************************************************
   Function: CmdFileSaveAll
   Description:
*/
void CmdFileSaveAll(void *pCtx)
{
  TCheckSaveContext CheckSaveContext;
  int nCurFile;

  CheckSaveContext.bCanceled = FALSE;
  CheckSaveContext.bCheck = FALSE;
  CheckSaveContext.wrkspace = CMDC_WRKSPACE(pCtx);

  nCurFile = GetTopFileNumber(pFilesInMemoryList);
  FileListForEach(pFilesInMemoryList, CheckSave, FALSE, &CheckSaveContext);
  SetTopFileByLoadNumber(pFilesInMemoryList, nCurFile);
}

/* ************************************************************************
   Function: CmdFileClose
   Description:
*/
void CmdFileClose(void *pCtx)
{
  char sExpandedNoname[_MAX_PATH];
  TFile *pFile;
  dispc_t *disp;

  pFile = CMDC_PFILE(pCtx);
  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  StoreRecoveryRecord(pFile);

  strcpy(sExpandedNoname, sNoname);
  GetFullPath(sExpandedNoname);

  if (pFile->bNew && filestrcmp(pFile->sFileName, sExpandedNoname) == 0 &&
    !pFile->bChanged && pFile->nNumberOfLines == 0)
  {
    RemoveMRUItem(pMRUFilesList, pFile->sFileName, 0);
    CheckToRemoveRecoveryFile(pFile, disp);  /* If everything is undone */
    RemoveTopFile(pFilesInMemoryList, pMRUFilesList, disp);
    return;
  }

  if (pFile->bChanged)
  {
    switch (ConsoleMessageProc(disp, NULL, MSG_WARNING | MSG_YESNOCANCEL, NULL, sAskSave))
    {
      case 0:  /* Yes or Enter */
        CmdFileSave(pCtx);
        break;
      case 1:  /* No */
        pFile->bChanged = FALSE;  /* This will remove the recovery file below */
        break;
      case 2:  /* Esc/Cancel */
        return;  /* Don't close the file */
    }
  }

  AddFileToMRUList(pMRUFilesList, pFile->sFileName,
    pFile->nCopy, TRUE, FALSE, pFile->bForceReadOnly,
    pFile->nCol, pFile->nRow, pFile->nTopLine, FALSE);

  CheckToRemoveRecoveryFile(pFile, disp);
  RemoveTopFile(pFilesInMemoryList, pMRUFilesList, disp);
}

/* ************************************************************************
   Function: UpdateMRUEntry
   Description:
     A call-back function to update MRU entry for a file.
*/
static BOOLEAN UpdateMRUEntry(TFile *pFile, void *pContext)
{
  ASSERT(VALID_PFILE(pFile));

  if (pFile->bNew)
  {
    /*
    For files that are new there is no a MRU entry.
    Check whether the file is empty or not empty but not saved on disk.
    */
    if (FILE_IS_EMPTY(pFile) || pFile->bChanged)
      return TRUE;  /* Do not store the file in the MRU list */
    /* Add to MRU list */
    AddFileToMRUList(pMRUFilesList, pFile->sFileName,
      pFile->nCopy, FALSE, (BOOLEAN)(pFile == GetCurrentFile()),
      pFile->bForceReadOnly, pFile->nCol, pFile->nRow, pFile->nTopLine, FALSE);
  }
  else  /* Update MRU entry */
    AddFileToMRUList(pMRUFilesList, pFile->sFileName,
      pFile->nCopy, FALSE, (BOOLEAN)(pFile == GetCurrentFile()),
      pFile->bForceReadOnly, pFile->nCol, pFile->nRow, pFile->nTopLine, TRUE);

  return TRUE;
}

/* ************************************************************************
   Function: UnsetChangedFlag
   Description:
     A call-back function to set bChanged flag to FALSE
     for all the files in memory.
*/
static BOOLEAN UnsetChangedFlag(TFile *pFile, void *pContext)
{
  ASSERT(VALID_PFILE(pFile));
  pFile->bChanged = FALSE;  /* This will remove the rec file at DisposeFileProc() as well */
  return TRUE;
}

/* ************************************************************************
   Function: CmdFileExit
   Description:
*/
void CmdFileExit(void *pCtx)
{
  TCheckSaveContext CheckSaveContext;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  /* Check to save the changed files before exit */
  CheckSaveContext.bCanceled = FALSE;
  CheckSaveContext.bCheck = TRUE;
  CheckSaveContext.wrkspace = CMDC_WRKSPACE(pCtx);
  FileListForEach(pFilesInMemoryList, CheckSave, FALSE, &CheckSaveContext);
  if (CheckSaveContext.bCanceled)
    return;

  /* We need to change all the bChanged flags to FALSE */
  FileListForEach(pFilesInMemoryList, UnsetChangedFlag, FALSE, NULL);

  /*
  Update MRU entries for each of the files in memory
  upon exiting the program
  */
  FileListForEach(pFilesInMemoryList, UpdateMRUEntry, FALSE, NULL);

  /*
  Dispose files from the memory
  */
  RemoveBlankFile(CheckToRemoveBlankFile(), disp);  /* "noname" is special */
  while (RemoveTopFile(pFilesInMemoryList, pMRUFilesList, disp))
    ;

  bBlockMarkMode = FALSE;
  bIncrementalSearch = FALSE;
  bPreserveIncrementalSearch = FALSE;
  bQuit = TRUE;
}

/* ************************************************************************
   Function: LoadMRUFile2
   Description:
     A wrap function of LoadMRUFile.
     It takes care to remove the initial blank file, if the editor
     is in this pristine state.
*/
static void
LoadMRUFile2(int nFile,
             TFileList *pFileList,
             TMRUList *pMRUList,
             TDocType *pDocTypeSet,
             dispc_t *disp)
{
  BOOLEAN bRemove;

  bRemove =  CheckToRemoveBlankFile();
  LoadMRUFile(nFile, pFilesInMemoryList, pMRUFilesList, pDocTypeSet, disp);
  RemoveBlankFile(bRemove, disp);
}

/* ************************************************************************
   Function: CmdFileOpenMRU1
   Description:
*/
void CmdFileOpenMRU1(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (pMRUFilesList->nNumberOfItems > 0)
    LoadMRUFile2(0, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);
}

/* ************************************************************************
   Function: CmdFileOpenMRU2
   Description:
*/
void CmdFileOpenMRU2(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (pMRUFilesList->nNumberOfItems > 1)
    LoadMRUFile2(1, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);
}

/* ************************************************************************
   Function: CmdFileOpenMRU3
   Description:
*/
void CmdFileOpenMRU3(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (pMRUFilesList->nNumberOfItems > 2)
    LoadMRUFile2(2, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);
}

/* ************************************************************************
   Function: CmdFileOpenMRU4
   Description:
*/
void CmdFileOpenMRU4(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (pMRUFilesList->nNumberOfItems > 3)
    LoadMRUFile2(3, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);
}

/* ************************************************************************
   Function: CmdFileOpenMRU5
   Description:
*/
void CmdFileOpenMRU5(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (pMRUFilesList->nNumberOfItems > 4)
    LoadMRUFile2(4, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);
}

/* ************************************************************************
   Function: CmdFileOpenMRU6
   Description:
*/
void CmdFileOpenMRU6(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (pMRUFilesList->nNumberOfItems > 5)
    LoadMRUFile2(5, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);
}

/* ************************************************************************
   Function: CmdFileOpenMRU7
   Description:
*/
void CmdFileOpenMRU7(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (pMRUFilesList->nNumberOfItems > 6)
    LoadMRUFile2(6, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);
}

/* ************************************************************************
   Function: CmdFileOpenMRU8
   Description:
*/
void CmdFileOpenMRU8(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (pMRUFilesList->nNumberOfItems > 7)
    LoadMRUFile2(7, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);
}

/* ************************************************************************
   Function: CmdFileOpenMRU9
   Description:
*/
void CmdFileOpenMRU9(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (pMRUFilesList->nNumberOfItems > 8)
    LoadMRUFile2(8, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);
}

/* ************************************************************************
   Function: CmdFileOpenMRU10
   Description:
*/
void CmdFileOpenMRU10(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  if (pMRUFilesList->nNumberOfItems > 9)
    LoadMRUFile2(9, pFilesInMemoryList, pMRUFilesList, DocumentTypes, disp);
}

/* ************************************************************************
   Function: ProcessCommandLine
   Description:
     Loads all the files from the command line
*/
void ProcessCommandLine(int argc, char **argv, dispc_t *disp)
{
  int i;
  int nFileNumber;
  char sResult[_MAX_PATH];
  char sDPath[_MAX_PATH];
  char sDFile[_MAX_PATH];
  BOOLEAN bRemoveFlag;

  for (i = 1; i < argc;	++i)
  {
    nFileNumber = SearchFileList(pFilesInMemoryList, argv[i], 0);
    if (nFileNumber != -1)
    {
      SetTopFileByLoadNumber(pFilesInMemoryList, nFileNumber);
      continue;
    }

    strcpy(sResult, argv[i]);

    /*
    Analize the name
    1. If it is a directory name enter the directory with sDefaultMask by
    invoking FileMenu.
    2. If has wildcards invoke to process the FileMenu as well.
    */
    if (!FSplit(sResult, sDPath, sDFile, sAllMask, TRUE, FALSE))
    {
      ConsoleMessageProc(disp, NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, sResult, NULL);
      continue;
    }

    strcpy(sResult, sDPath);
    AddTrailingSlash(sResult);
    strcat(sResult, sDFile);

    if (HasWild(sResult))
    {
      if (!FileMenu(sResult, disp))
        return;
    }

    bRemoveFlag = CheckToRemoveBlankFile();
    LoadFile(sResult,
             pFilesInMemoryList, pMRUFilesList, DocumentTypes,
             FALSE, FALSE, disp);
    RemoveBlankFile(bRemoveFlag, disp);
  }
}

/*
-------------------------------------------
Small diagnostic pseudo terminal functions.
-------------------------------------------
*/

static char *screen[256];
static int term_x;
static int term_y;
static int last_ln;
static char spaces[512];
#ifdef _DEBUG
static BOOLEAN bInit = FALSE;
#define TERM_INIT()  bInit
#else
#define TERM_INIT()  (1)
#endif

/* ************************************************************************
   Function: CloseSmallTerminal
   Description:
     Closes the small terminal.
*/
void CloseSmallTerminal(void)
{
  int i;

  ASSERT(TERM_INIT());

  for (i = 0; i < 256; ++i)
    if (screen[i] != NULL)
      s_free(screen[i]);
  #ifdef _DEBUG
  bInit = FALSE;
  #endif
}

/* ************************************************************************
   Function: OpenSmallTerminal
   Description:
     Opens a small pseudo terminal.
     Q: Why pseudo?
     A: Because all the scroll and cursor positioning functions
     are done by hand.
     Q: Why not using the natural printf() functionality?
     A: Because when working in UNIX terminals, returning natural
     terminal modes disables our editor keyboard handling? Furthermore
     doing this by hand, gives an opportunity to provide some
     output beauty stuff.
*/
void OpenSmallTerminal(dispc_t* disp)
{
  int i;

  ASSERT(disp_wnd_get_height(disp) < 256);
  ASSERT(!TERM_INIT());

  #if _DEBUG
  bInit = TRUE;
  #endif

  memset(screen, 0, sizeof(screen));  /* Set all to null */

  for (i = 0; i < disp_wnd_get_height(disp); ++i)
  {
    screen[i] = alloc(disp_wnd_get_width(disp) * 2);
    if (screen[i] == NULL)
    {
      CloseSmallTerminal();
      return;
    }
    screen[i][0] = '\0';
  }

  for (i = 0; i < disp_wnd_get_height(disp); ++i)
    disp_fill(disp, ' ', GetColor(coEnterLnPrompt), 0, i,
      disp_wnd_get_width(disp));

  term_x = 0;
  term_y = 0;
  last_ln = 0;
  memset(spaces, ' ', 510);
  spaces[511] = '\0';
}

/* ************************************************************************
   Function: CalcFlexLen
   Description:
     Calculates Flex write string length. That means not to count
     the '~' characters.
*/
static int CalcFlexLen(const char *s)
{
  const char *p;
  int c;

  if (s == 0)  /* If the address of the string is NULL */
    return 0;

  c = 0;
  for (p = s; *p; ++p)
    if (*p != '~')
      c++;

  return c;
}

/* ************************************************************************
   Function: PrintString
   Description:
     Displays a string on the small terminal.
*/
void PrintString(dispc_t *disp, const char *fmt, ...)
{
  va_list ap;
  char buf[1024];
  char *p;
  char *last;
  int i;
  int nlastline;
  int len;

  ASSERT(TERM_INIT());

  va_start(ap, fmt);
  vsprintf(buf, fmt, ap);
  va_end( ap );

  p = buf;
  last = p;
  nlastline = term_y;
  while (*p != '\0')
  {
    if (*p == '\n')
    {
      if (term_y == disp_wnd_get_height(disp) - 1)
      {
        /* do a scroll: copy all the lines to an upper position */
        for (i = 1; i <= term_y; ++i)
          strcpy(screen[i - 1], screen[i]);
        screen[term_y][0] = '\0';  /* plus clear last line on the screen */
        disp_fill(disp, ' ', GetColor(coEnterLnPrompt), 0,
          disp_wnd_get_height(disp) - 1, disp_wnd_get_width(disp));
        --term_y;  /* The work line is positionized a line up */
        nlastline = 0;  /* redisplay whole the screen */
      }

      *p = '\0';  /* from last up to *p is a line to be displayed */
      strcat(screen[term_y], last);
      last = p + 1;
      term_x = 0;
      len = CalcFlexLen(screen[term_y]);
      if (len < disp_wnd_get_width(disp))  /* Right pad the line with spaces */
        strcat(screen[term_y],
          &spaces[512 - (disp_wnd_get_width(disp) - len) - 1]);
      screen[term_y][disp_wnd_get_width(disp) - 1] = '\0';
      term_y += 1;
    }
    ++p;
  }

  if (last == buf)
  {
    /* This line has no newline marker '\n' */
    strcat(screen[term_y], last);
    term_x = CalcFlexLen(screen[term_y]);
    disp_flex_write(disp, screen[term_y], 0, term_y,
      GetColor(coEnterLnBraces), GetColor(coEnterLn));
  }

  for (i = nlastline; i < term_y; ++i)
    disp_flex_write(disp, screen[i], 0, i, GetColor(coTerm1), GetColor(coTerm2));

  disp_cursor_goto_xy(disp, term_x, term_y);
}

/* ************************************************************************
   Function: DumpBlockList
   Description:
     Dumps block list with control purposes
   On entry:
     _nstart_ is the number of lines already displayed when DumpBlockList()
     is invoked
   Returns:
     total number of references to all the blocks in the lists
*/
static int DumpBlockList(const TListRoot *blist,
  int *nstart, int *nTotalSize, dispc_t *disp)
{
  const TFileBlock *pFileBlock;
  int n;
  int count;

  *nTotalSize = 0;

  if (IS_LIST_EMPTY(blist))
  {
    PrintString(disp, "blist is empty\n");
    return 0;
  }

  pFileBlock = (TFileBlock *)(blist->Flink);
  n = 0;
  count = 0;
  while (!END_OF_LIST(blist, &pFileBlock->link))
  {
    count += pFileBlock->nRef;
    *nTotalSize += pFileBlock->nBlockSize;
    PrintString(disp, "block %#x, %d references\n", (int)(pFileBlock + 1), pFileBlock->nRef);
    if (!DiagContinue(*nstart + n++, disp))
      break;
    pFileBlock = (TFileBlock *)(pFileBlock->link.Flink);
  }
  PrintString(disp, "%d blocks, %d bytes occupied\n", n, *nTotalSize);

  *nstart += n;

  return count;
}

/* ************************************************************************
   Function: DumpFileBlock
   Description:
*/
static void DumpFileBlock(const char *c, dispc_t *disp)
{
  TFileBlock *pFileBlock;

  pFileBlock = (TFileBlock *)c - 1;
  PrintString(disp, "block %#x, %d references\n", (int)(pFileBlock + 1), pFileBlock->nRef);
}

/* ************************************************************************
   Function: GetEOLType
   Description:
*/
static char *GetEOLType(int nEOLType)
{
  switch (nEOLType)
  {
    case CRLFtype:
      return "CR/LF end-of-line";
    case LFtype:
      return "LF end-of-line";
    case CRtype:
      return "CR end-of-line";
    default:
      ASSERT(0);  /* invalid EOL type */
  }
  return "bad value";
}

/* ************************************************************************
   Function: GetBool
   Description:
*/
static char *GetBool(BOOLEAN bFlag)
{
  return bFlag ? "TRUE " : "FALSE";
}

/* ************************************************************************
   Function: DumpRearrangeLines
   Description:
*/
void DumpRearrangeLines(int *pLines, int nNumberOfLines, dispc_t *disp)
{
  int i;

  for (i = 0; i < nNumberOfLines; ++i)
  {
    PrintString(disp, "%d\n", pLines[i]);
    if (!DiagContinue(i + 2, disp))
      break;
  }
}

/* ************************************************************************
   Function: DumpLines
   Description:
     pIndex -- array of pointers to a ASCIIZ strings
     nNumberOfLines -- numbe of lines contained in pIndex
     nLines -- how much lines were displayed before calling DumpLines.
*/
static void DumpLines(const TLine *pIndex,
  unsigned int nNumberOfLines, int *nLines, dispc_t *disp)
{
  const TLine *p;
  char sBuf[1024];
  char *d;
  char *s;
  int nCol;
  int nNewCol;
  int i;
  int nLeader;
  int nMagnitude;
  int nNumberWidth;

  ASSERT(disp_wnd_get_width(disp) < 1024);

  if (pIndex == NULL)
    return;

  p = pIndex;
  nMagnitude = nNumberOfLines;
  nNumberWidth = 1;
  while (nMagnitude /= 10)
    ++nNumberWidth;
  while (nNumberOfLines--)
  {
    s = p->pLine;
    ASSERT(p->nLen == (int)strlen(p->pLine));
    sprintf(sBuf, "%-*d:", nNumberWidth, (int)(p - pIndex));
    d = strchr(sBuf, '\0');
    nLeader = strlen(sBuf);
    nCol = 0;
    while (*s)
    {
      if (*s == ' ')
      {
        *d++ = '\xfa';
        ++s;
        ++nCol;
      }
      else
        if (*s == '\t')
        {
          nNewCol = CalcTab(nCol);
          for (i = nCol; i < nNewCol; ++i)
            *d++ = '\xf9';
          nCol = nNewCol;
          ++s;
        }
        else
        {
          *d = *s;
          ++d;
          ++s;
          ++nCol;
        }
      if (nCol + nLeader == disp_wnd_get_width(disp) - 3)
      {
        *d++ = '>';
        *d++ = '>';
        break;
      }
    }
    *d = '\0';
    PrintString(disp, "%s\n", sBuf);
    ++p;
    if (!DiagContinue(++*nLines, disp))
      break;
  }
}

/* ************************************************************************
   Function: CalcFileSize
   Description:
*/
static int CalcFileSize(const TFile *pFile)
{
  int i;
  int nSize;
  int nEOLSize;

  nSize = 0;
  nEOLSize = pFile->nEOLType == CRLFtype ? 2 : 1;
  for (i = 0; i < pFile->nNumberOfLines; ++i)
    nSize += GetLine(pFile, i)->nLen + nEOLSize;

  return nSize;
}


/* ************************************************************************
   Function: DumpFile
   Description:
     Dumps file with control purposes.
     To be called from the diagnostic menu.
*/
void DumpFile(const TFile *pFile, dispc_t* disp)
{
  int nRef;
  int nLines;
  int nAllocatedSize;
  int nActualSize;

  ASSERT(VALID_PFILE(pFile));

  PrintString(disp, "         ~file:~ %s\n", pFile->sFileName);
  PrintString(disp, "        ~title:~ %s\n", pFile->sTitle);
  PrintString(disp, "~recovery file:~ ");
  if (pFile->sRecoveryFileName[0] == 0)
  {
    PrintString(disp, "not supported for copy >0\n");
    ASSERT(pFile->nCopy > 0);
  }
  else
  {
    PrintString(disp, "%s\n", pFile->sRecoveryFileName);
    ASSERT(pFile->nCopy == 0);
  }
  PrintString(disp, "         ~copy:~ %d\n", pFile->nCopy);
  PrintString(disp, "      ~EOLType:~ (%d) %s; ~nCR~: %d ~nLF~: %d ~nCRLF~: %d ~nZ~: %d\n",
    pFile->nEOLType, GetEOLType(pFile->nEOLType), pFile->nCR, pFile->nLF, pFile->nCRLF, pFile->nZero);
  PrintString(disp, "~LastWriteTime:~ %d/%d/%d, %d:%d.%d\n",
    pFile->LastWriteTime.month, pFile->LastWriteTime.day, pFile->LastWriteTime.year,
    pFile->LastWriteTime.hour, pFile->LastWriteTime.min,
    pFile->LastWriteTime.sec);
  PrintString(disp, "     ~filesize:~ %d bytes (reported from disk)\n", pFile->nFileSize);
  PrintString(disp, "        ~lines:~ %d lines\n", pFile->nNumberOfLines);
  PrintString(disp, "     ~col, row:~ %d:%d\n", pFile->nCol, pFile->nRow);
  PrintString(disp, "     ~top line:~ %d\n", pFile->nTopLine);
  PrintString(disp, "   ~write edge:~ %d\n", pFile->nWrtEdge);
  PrintString(disp, "    ~has block:~ %s        ~block:~ %d:%d-%d:%d, %#x\n", GetBool(pFile->bBlock),
    pFile->nStartLine, pFile->nStartPos, pFile->nEndLine, pFile->nEndPos,
    pFile->blockattr);
  PrintString(disp, "      ~changed:~ %s     ~new file:~ %s\n",  GetBool(pFile->bChanged), GetBool(pFile->bNew));
  PrintString(disp, "    ~read only:~ %s    ~force r/o:~ %s\n", GetBool(pFile->bReadOnly), GetBool(pFile->bForceReadOnly));
  PrintString(disp, "   ~rec stored:~ %s\n", GetBool(pFile->bRecoveryStored));
  PrintString(disp, "  ~undo id cnt:~ %-4d  ~undo records:~ %d\n", pFile->nUndoIDCounter, pFile->nNumberOfRecords);
  #ifdef _DEBUG
  if (pFile->pUndoIndex != NULL)
    ASSERT(pFile->nNumberOfRecords == _TArrayCount(pFile->pUndoIndex));
  #endif

  nLines = 17;
  nRef = DumpBlockList(&pFile->blist, &nLines, &nAllocatedSize, disp);

  nActualSize = CalcFileSize(pFile);
  if (nAllocatedSize != 0)
  {
    PrintString(disp, "Current file size in memory is %d bytes, %d%% wasted space\n",
      nActualSize, (nAllocatedSize - nActualSize) * 100 / nAllocatedSize);
  }
  ++nLines;

  if (nRef != pFile->nNumberOfLines)
    PrintString(disp, "!!! total number of block references doesn't match the number of lines !!!\n");

  if (pFile->pIndex)
  {
    if (_TArrayCount(pFile->pIndex) != pFile->nNumberOfLines)
      PrintString(disp, "!!! total number of lines doesn't match the indexed number of lines\n");
    DumpLines(pFile->pIndex, pFile->nNumberOfLines, &nLines, disp);
  }
}

/* ************************************************************************
   Function: DumpBlock
   Description:
*/
void DumpBlock(const TBlock *pBlock, int nHeaderLines, dispc_t *disp)
{
  int nLines;

  if (pBlock == NULL)
  {
    PrintString(disp, "empty\n");
    return;
  }

  /*TArrayDumpInfo(pBlock->pIndex);*/  /* Detailed only! */
  DumpFileBlock(pBlock->pBlock, disp);
  PrintString(disp, "nNumberOfLines: %d\n", pBlock->nNumberOfLines);
  PrintString(disp, "nEOLType: %d\n", pBlock->nEOLType);
  PrintString(disp, "blockattr: ");
  if (pBlock->blockattr & COLUMN_BLOCK)
    PrintString(disp, "_column_block_ ");
  else
    PrintString(disp, "_character_block_ ");
  PrintString(disp, "\n");

  nLines = 3 + nHeaderLines;  /* 2 lines already displaied, +1 for the menu line */
  DumpLines(pBlock->pIndex, pBlock->nNumberOfLines, &nLines, disp);
}

/* ************************************************************************
   Function: DumpMRUList
   Description:
*/
void DumpMRUList(TMRUList *pMRUList, dispc_t *disp)
{
  int nCount;
  TMRUListItem *pMRUItem;

  PrintString(disp, "MRUList (%p), NumberOfItems: %d, MaxNumberOfItems: %d\n",
    pMRUList, pMRUList->nNumberOfItems, pMRUList->nMaxNumberOfItems);

  ASSERT(pMRUList != NULL);

  if (IS_LIST_EMPTY(&pMRUList->flist))
  {
    ASSERT(pMRUList->nNumberOfItems == 0);

    PrintString(disp, "empty\n");
    return;
  }

  PrintString(disp, "----------------------------------------------\n");
  PrintString(disp, "Col  Row  TopLine Closed Current Copy FileName\n");
  PrintString(disp, "----------------------------------------------\n");
  nCount = 6;

  pMRUItem = (TMRUListItem *)(pMRUList->flist.Flink);
  while (!END_OF_LIST(&pMRUList->flist, &pMRUItem->flink))
  {
    PrintString(disp, "%-5d%-5d%-8d%-7d%-8d%-5d%s\n",
      pMRUItem->nCol,
      pMRUItem->nRow,
      pMRUItem->nTopLine,
      pMRUItem->bClosed,
      pMRUItem->bCurrent,
      pMRUItem->nCopy,
      pMRUItem->sFileName);

    if (!DiagContinue(++nCount, disp))
      return;
    pMRUItem = (TMRUListItem *)(pMRUItem->flink.Flink);
  }
}

/* ************************************************************************
   Function: DumpUndoIndex
   Description:
*/
void DumpUndoIndex(TFile *pFile, dispc_t *disp)
{
  int i;
  TUndoRecord *pUndoRec;
  char *p;
  DWORD nKey;
  disp_event_t ev;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->nNumberOfRecords == 0)
  {
    PrintString(disp, "Undo record index is empty\n");
    return;
  }

  for (i = 0; i < pFile->nNumberOfRecords; ++i)
  {
    pUndoRec = &pFile->pUndoIndex[i];
    if (pUndoRec->nUndoLevel == 1)  /* Separate the atoms with a line */
      PrintString(disp, "-------------------------------------------------------------------------------\n");
    else
      PrintString(disp, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    PrintString(disp, "operation: ");
    switch (pUndoRec->nOperation)
    {
      case acINSERT:
        PrintString(disp, "INSERT");
        break;
      case acDELETE:
        PrintString(disp, "DELETE");
        break;
      case acREPLACE:
        PrintString(disp, "REPLACE");
        break;
      case acREARRANGE:
        PrintString(disp, "REARRANGE");
        break;
      default:
        ASSERT(0);
    }
    if (pUndoRec->bUndone)
      PrintString(disp, " (UNDONE)");
    PrintString(disp, "\n");
    PrintString(disp, "bRecoveryStore: %s\n", GetBool(pUndoRec->bRecoveryStore));
    PrintString(disp, "------ before[%d]: cursor pos (%d:%d), selection (%d:%d / %d:%d) %d lines ------\n",
      pUndoRec->nUndoBlockID,
      pUndoRec->before.nCol, pUndoRec->before.nRow,
      pUndoRec->before.nStartLine, pUndoRec->before.nStartPos,
      pUndoRec->before.nEndLine, pUndoRec->before.nEndPos,
      pUndoRec->before.nNumberOfLines);
    PrintString(disp, "------ after [%d]: cursor pos (%d:%d), selection (%d:%d / %d:%d) %d lines ------\n",
      pUndoRec->nUndoBlockID,
      pUndoRec->after.nCol, pUndoRec->after.nRow,
      pUndoRec->after.nStartLine, pUndoRec->after.nStartPos,
      pUndoRec->after.nEndLine, pUndoRec->after.nEndPos,
      pUndoRec->after.nNumberOfLines);
    PrintString(disp, "press 'b' to dump data\n");
_waitkey:
    do
    {
      disp_event_read(disp, &ev);
    }
    while (ev.t.code != EVENT_KEY);
    nKey = ev.e.kbd.key;
    if (nKey == 0xffff)
      goto _waitkey;
    if (ASC(nKey) == 'b' || ASC(nKey) == 'B')
      switch (pUndoRec->nOperation)
      {
        case acREARRANGE:
          PrintString(disp, "start line: %d, end line: %d\n", pUndoRec->nStart, pUndoRec->nEnd);
          DumpRearrangeLines(pUndoRec->pData,
            pUndoRec->nEnd - pUndoRec->nStart + 1, disp);
          break;
        default:
          if (pUndoRec->pData == NULL)
            PrintString(disp, "no data\n");
          else
          {
            PrintString(disp, "data (%d:%d / %d:%d):\n",
              pUndoRec->nStart, pUndoRec->nStartPos,
              pUndoRec->nEnd, pUndoRec->nEndPos);
            DumpBlock(pUndoRec->pData, 1, disp);
          }
      }
    p = "";
    if (pUndoRec->nUndoLevel == 1)
     p = "--- end of atom ";
    PrintString(disp, "------ lvl: %d %s------\n\n", pUndoRec->nUndoLevel, p);
    if (!DiagContinue(0, disp))
      return;
  }
}

static int nBMCount;
struct printc
{
  dispc_t* disp;
};

typedef struct printc printc_t;

/* ************************************************************************
   Function: PrintBookmark
   Description:
*/
static BOOLEAN PrintBookmark(TMarkLocation *pMark,
  int nRow, void *pContext)
{
  char sShortName[_MAX_PATH];
  char sContent[2048];
  printc_t *printc;

  ASSERT(VALID_PMARKLOCATION(pMark));

  printc = pContext;

  sShortName[0] = '\0';
  sContent[0] = '\0';
  if (pMark->psContent != NULL)
    strcpy(sContent, pMark->psContent);
  if (pMark->pFileName != NULL)
    ShrinkPath(pMark->pFileName->sFileName, sShortName, 0, TRUE);
  PrintString(printc->disp, "%s: (%d) %d, %s %s\n", sShortName, pMark->nOffset, nRow + 1,
    ((TBookmarksSet *)pMark->pSet)->sTitle, sContent);
  if (!DiagContinue(++nBMCount, printc->disp))
    return FALSE;
  return TRUE;
}

/* ************************************************************************
   Function: DumpFuncList
   Description:
     Dumps all the functions in a file.
     To be called from the diagnostic menu.
*/
void DumpFuncList(TFile *pFile, dispc_t *disp)
{
  TFunctionName stFunctions[2];
  int nNumFunctions;
  int nCurLine;
  int nCurPos;
  int i;
  char sFunction[128];
  char *sLine;
  int nNameLen;

  nCurLine = 0;
  nCurPos = 0;

_scan:
  nNumFunctions = FunctionNameScan(pFile, nCurLine, nCurPos,
    pFile->nNumberOfLines, 0,
    stFunctions, _countof(stFunctions));
  if (nNumFunctions == 0)
    return;

  for (i = 0; i < nNumFunctions; ++i)
  {
    memset(sFunction, 0, sizeof(sFunction));
    sLine = GetLineText(pFile, stFunctions[i].nLine);
    nNameLen = stFunctions[i].nNameLen;
    if (nNameLen > sizeof(sFunction) - 1)
      nNameLen = sizeof(sFunction) - 1;
    strncpy(sFunction,
      sLine + stFunctions[i].nNamePos, nNameLen);
    PrintString(disp, "%s\n", sFunction);
  }

  nCurLine = stFunctions[i - 1].nFuncEndLine;
  nCurPos = stFunctions[i - 1].nFuncEndPos;
  goto _scan;
}

/* ************************************************************************
   Function: DumpBookmarks
   Description:
*/
void DumpBookmarks(dispc_t *disp)
{
  nBMCount = 1;
  BMListForEach(PrintBookmark, NULL, FALSE, NULL);
  PrintString(disp, "\n");
}

/* ************************************************************************
   Function: DumpSearchReplacePattern
   Description:
*/
void DumpSearchReplacePattern(TSearchContext *pstSearchContext, dispc_t *disp)
{
  int i;

  PrintString(disp, "~options:~ ");
  PrintString(disp, "-i%s ", pstSearchContext->bCaseSensitive ? "-" : "+");
  PrintString(disp, "-r%s ", pstSearchContext->bRegularExpr ? "+" : "-");
  PrintString(disp, "-b%s ", pstSearchContext->nDirection == -1 ? "+" : "-");
  PrintString(disp, "\n");
  PrintString(disp, "~search:~ %s\n", pstSearchContext->sSearch);
  PrintString(disp, "~replace:~\n");
  if (pstSearchContext->nNumRepl == 0)
    PrintString(disp, "  none\n");
  for (i = 0; i < pstSearchContext->nNumRepl; ++i)
    if (pstSearchContext->Repl[i].type == 0)
      PrintString(disp, "  ~string:~ %s\n", pstSearchContext->Repl[i].data.sReplString);
    else
    {
      PrintString(disp, "  ~reference:~ %d: %s\n",
        pstSearchContext->Repl[i].data.pattern,
        pstSearchContext->Substrings[pstSearchContext->Repl[i].data.pattern]);
    }
  PrintString(disp, "\n");
}

/* ************************************************************************
   Function: DumpContainersTree
   Description:
*/
void DumpContainersTree(TContainer *pCont, int indent, dispc_t *disp)
{
  ASSERT(VALID_PCONTAINER(pCont));

  PrintString(disp, "%*c%p: %d %d %d %d", indent, ' ', pCont,
    pCont->x, pCont->y, pCont->nWidth, pCont->nHeight);
  if (pCont->pSub1)
  {
    PrintString(disp, "\n");
    DumpContainersTree(pCont->pSub1, indent + 4, disp);
    DumpContainersTree(pCont->pSub2, indent + 4, disp);
    return;
  }
  PrintString(disp, " view:%p:%d %d %d %d\n",
    pCont->pView,
    pCont->pView->x, pCont->pView->y,
    pCont->pView->nWidth, pCont->pView->nHeight);
}

/* ************************************************************************
   Function: CmdHelpAbout
   Description:
*/
void CmdHelpAbout(void *pCtx)
{
  DWORD Key;
  disp_event_t ev;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));
  OpenSmallTerminal(disp);

  PrintString(disp, "WW text editor\n");
  PrintString(disp, "-------------\n");
  PrintString(disp, "~version:~ %d.%d.%d\n", vermaj, vermin, ver_revision);
  PrintString(disp, "~build:~ [%d]\n", verbld);
  PrintString(disp, "~configuration file version:~ %d\n", vercfg);
  PrintString(disp, "~build date/time:~ %s; %s\n", datebld, timebld);
  PrintString(disp, "~definitions:~\n");
#ifdef _DEBUG
  PrintString(disp, "_DEBUG_\n");
#endif
#ifdef HEAP_DBG
  PrintString(disp, "HEAP_DBG=%d\n", HEAP_DBG);
#endif
#ifdef WIN32
  PrintString(disp, "WIN32\n");
#endif
#ifdef _MSC_VER
  PrintString(disp, "_MSC_VER=%d\n", _MSC_VER);
#endif
#ifdef MSDOS
  PrintString(disp, "MSDOS\n");
#endif
#ifdef DJGPP
  PrintString(disp, "DJGPP=%d, DJGPP_MINOR=%d\n", DJGPP, DJGPP_MINOR);
#endif
#ifdef LINUX
  PrintString(disp, "LINUX\n");
#endif
#ifdef UNIX
  PrintString(disp, "UNIX\n");
#endif
#ifdef _NON_TEXT
  PrintString(disp, "_NON_TEXT\n");
#endif

  PrintString(disp, "\n");

_main_menu:
  PrintString(disp, "~[A]~uthor ~[C]~redits ~[L]~icense L~[o]~go\n");

_wait_key:
  do
  {
    disp_event_read(disp, &ev);
  }
  while (ev.t.code != EVENT_KEY);
  Key = ev.e.kbd.key;

  if (Key == KEY(0, kbEsc))
  {
    CloseSmallTerminal();
    GetCurrentFile()->bUpdatePage = TRUE;
    return;
  }

  switch (ASC(Key))
  {
    case 'a':
    case 'A':
      PrintString(disp, "Written by Petar Marinov\n");
      PrintString(disp, "Portions written by Tzvetan Mikov\n");
      PrintString(disp, "Copyright (c) 1995-2003 Petar Marinov\n");
      PrintString(disp, "Regular expressions written by Philip Hazel,\n");
      PrintString(disp, "  University of Cambridge Computing Service, England.\n");
      PrintString(disp, "  Copyright (c) 1997-2000 University of Cambridge\n");
      PrintString(disp, "web site:\nhttp://geocities.com/h2428/\n");
      PrintString(disp, "\n");
      break;

    case 'c':
    case 'C':
      PrintString(disp, "a lot of help from friends:\n");
      PrintString(disp, "\n");
      break;

    case 'l':
    case 'L':
      PrintString(disp, "This software is distributed under the conditions of this BSD style license.\n\n");

      PrintString(disp, "Copyright (c)\n");
      PrintString(disp, "1995-2003\n");
      PrintString(disp, "Petar Marinov\n\n");

      PrintString(disp, "All rights reserved.\n\n");

      PrintString(disp, "Redistribution and use in source and binary forms, with or without\n");
      PrintString(disp, "modification, are permitted provided that the following conditions\n");
      PrintString(disp, "are met:\n");
      PrintString(disp, "1. Redistributions of source code must retain the above copyright\n");
      PrintString(disp, "   notice, this list of conditions and the following disclaimer.\n");
      PrintString(disp, "2. Redistributions in binary form must reproduce the above copyright\n");
      PrintString(disp, "   notice, this list of conditions and the following disclaimer in the\n");
      PrintString(disp, "   documentation and/or other materials provided with the distribution.\n");
      PrintString(disp, "3. The name of the author may not be used to endorse or promote products\n");
      PrintString(disp, "   derived from this software without specific prior written permission.\n\n");

      PrintString(disp, "THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR\n");
      PrintString(disp, "IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES\n");
      PrintString(disp, "OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.\n");
      PrintString(disp, "IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,\n");
      PrintString(disp, "INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT\n");
      PrintString(disp, "NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n");
      PrintString(disp, "DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
      PrintString(disp, "THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
      PrintString(disp, "(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF\n");
      PrintString(disp, "THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\n");
      break;

    case 'o':
    case 'O':
      PrintString(disp, "   ____________\n");
      PrintString(disp, "  < ww_is_cool >\n");
      PrintString(disp, "   ------------\n");
      PrintString(disp, "           \\  ^__^\n");
      PrintString(disp, "            \\ (oo)\\_______\n");
      PrintString(disp, "              (__)\\       )\\/\\\n");
      PrintString(disp, "                  ||----w |\n");
      PrintString(disp, "                  ||     ||\n\n");
      break;

    default:
      goto _wait_key;
  }
  goto _main_menu;
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

