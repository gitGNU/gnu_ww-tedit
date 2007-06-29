/*

File: file2.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 16th November, 1998
Descrition:
  Functions concerning file manipulation in context of platform
  indepentent work space components -- filelist, MRU list, recovery file.

*/

#include "global.h"
#include "scr.h"
#include "l2disp.h"
#include "path.h"
#include "filenav.h"
#include "mru.h"
#include "nav.h"
#include "l1opt.h"
#include "l1def.h"
#include "ini2.h"  /* bDontStoreINIFile */
#include "ini.h"  /* sINIFileName */
#include "memory.h"
#include "findf.h"
#include "doctype.h"
#include "undo.h"
#include "file2.h"

static int nRecFileCount;  /* Used by ComposeNewRecFileName() */
BOOLEAN bCoincides;
TFile *pTargetFile;

/* ************************************************************************
   Function: ChangeFileNameExtention
   Description:
     Changes the filename extention.
*/
static void ChangeFileNameExtention(char *psFileName, const char *psExt)
{
  char sPath[_MAX_PATH];
  char sFile[_MAX_PATH];
  char *p;

  ASSERT(psFileName != NULL);
  ASSERT(psExt != NULL);

  FSplit(psFileName, sPath, sFile, "ERROR", TRUE, TRUE);
  p = strrchr(sFile, '.');
  if (p == NULL || p == sFile)
    strcat(sFile, psExt);
  else
    strcpy(p, psExt);
  strcpy(psFileName, sPath);
  strcat(psFileName, sFile);
}

/* ************************************************************************
   Function: ComposeNewRecFileName
   Description:
     A call-back function that composes recovery file name
     for particular file. This function is called for
     all the files currently in memory.
     TODO: CONTEXT
*/
static BOOLEAN ComposeNewRecFileName(TFile *pFile, void *pContext)
{
  char sRecExtBuf[_MAX_PATH];

  ASSERT(VALID_PFILE(pFile));
  ASSERT(VALID_PFILE(pTargetFile));

  if (pFile == pTargetFile)
    return TRUE;  /* It's pointless to check for coincidence */

  if (filestrcmp(pFile->sRecoveryFileName, pTargetFile->sRecoveryFileName) == 0)
  {
    /*
    Coincidence found. Compose new name with ++nRecFileCount.
    */
    strcpy(pTargetFile->sRecoveryFileName, pTargetFile->sFileName);
    sprintf(sRecExtBuf, sRecExt2, nRecFileCount++);
    ChangeFileNameExtention(pTargetFile->sRecoveryFileName, sRecExtBuf);
    bCoincides = TRUE;
    return FALSE;  /* Restarts file names iteration */
  }

  return TRUE;
}

/* ************************************************************************
   Function: PrepareRecoveryFileName
   Description:
     Q: How a rec file name is prepared?
     A: Recovery file name is based on pFile->sFileName but with extention
     .rec or .r01, .r02, ... etc. Example: we have demo.c and demo.h in
     the same directory to resolve the collision if one of the files has
     alredy created recovery file demo.rec then the next will have demo.r01,
     by opening and examining demo.rec can be determined wich file is the
     target demo.c or demo.h, furthermore if there are 2 files with same
     name simultaneously loaded in the memory then the second will have
     extention .r01.
   TODO: Check the case of *.rec;
   IDEA: Why not making all *.r* it would be easier?
*/
static void PrepareRecoveryFileName(TFile *pFile, TFileList *pFileList)
{
  char sFileName[_MAX_PATH];  /* Loaded from recovery file stat line */
  char sRecExtBuf[_MAX_PATH];
  char sPath[_MAX_PATH];
  FF_DAT *f;
  struct findfilestruct r;
  BOOLEAN bRecFileFree;

  ASSERT(VALID_PFILE(pFile));

  /*
  Check the .rec file
  */
  strcpy(pFile->sRecoveryFileName, pFile->sFileName);
  ChangeFileNameExtention(pFile->sRecoveryFileName, sRec);

  bRecFileFree = TRUE;
  if (GetStatLine(pFile, sFileName,
    &pFile->nRecStatFileSize,
    &pFile->nRecStatMonth,
    &pFile->nRecStatDay,
    &pFile->nRecStatYear,
    &pFile->nRecStatHour,
    &pFile->nRecStatMin,
    &pFile->nRecStatSec))
  {
    if (stricmp(pFile->sFileName, sFileName) == 0)
    {
      /* The disk file .rec corresponds to this file */
      return;
    }
    bRecFileFree = FALSE;
  }

  /*
  .rec name is occupied by another file with same name. Now
  should be examined .r01, .r02, etc. posibilities.
  */
  strcpy(sRecExtBuf, pFile->sFileName);  /* Compose name.sRecExt2Mask */
  ChangeFileNameExtention(sRecExtBuf, sRecExt2Mask);
  FSplit(pFile->sFileName, sPath, sFileName /* Dummy */, "ERROR" /* Dummy */, TRUE, TRUE);
  f = find_open(sRecExtBuf, FFIND_FILES);
  if (f != NULL)
  {
    while (find_file(f, &r) == 0)
    {
      /* Compose sRecFileName getting the path of pFile->sFileName */
      strcpy(pFile->sRecoveryFileName, sPath);
      strcat(pFile->sRecoveryFileName, r.filename);
      if (GetStatLine(pFile, sFileName,
        &pFile->nRecStatFileSize,
        &pFile->nRecStatMonth,
        &pFile->nRecStatDay,
        &pFile->nRecStatYear,
        &pFile->nRecStatHour,
        &pFile->nRecStatMin,
        &pFile->nRecStatSec))
      {
        if (stricmp(pFile->sFileName, sFileName) == 0)
        {
          /* The disk file .rxx corresponds to this file */
          find_close(f);
          return;
        }
      }
    }
    find_close(f);
  }

  /*
  No recovery file at the disk for this	sFileName.
  Compose new name, taking on considerations all the files in memory.
  */
  nRecFileCount = 0;
  if (bRecFileFree)
  {
    strcpy(pFile->sRecoveryFileName, pFile->sFileName);
    ChangeFileNameExtention(pFile->sRecoveryFileName, sRec);
  }
  else
  {
_try_ext:
    strcpy(pFile->sRecoveryFileName, pFile->sFileName);
    sprintf(sRecExtBuf, sRecExt2, nRecFileCount);
    ChangeFileNameExtention(pFile->sRecoveryFileName, sRecExtBuf);
    ++nRecFileCount;
  }

  pTargetFile = pFile;
  do
  {
    bCoincides = FALSE;
    FileListForEach(pFileList, ComposeNewRecFileName, TRUE, NULL);
  }
  while (bCoincides);

  if (GetStatLine(pFile, sFileName,
    &pFile->nRecStatFileSize,
    &pFile->nRecStatMonth,
    &pFile->nRecStatDay,
    &pFile->nRecStatYear,
    &pFile->nRecStatHour,
    &pFile->nRecStatMin,
    &pFile->nRecStatSec))
  {
    if (stricmp(pFile->sFileName, sFileName) != 0)
      goto _try_ext;  /* This file is occupied for another session */
  }
  else
    if (FileExists(pFile->sRecoveryFileName))
      goto _try_ext;  /* If exists this is not a recovery file format */
}

/* ************************************************************************
   Function: PrepareFileNameTitle
   Description:
     Prepares full file name to look like:
     file (path\)
*/
void PrepareFileNameTitle(char *sFileName, int nCopy, char *sTitle, int nWidth,
  char *sViewID)
{
  char sFileNm[_MAX_PATH];
  char sPath[_MAX_PATH];
  char sShrunkPath[_MAX_PATH];
  char *p;
  char sCopy[6];
  int  nFWidth;

  ASSERT(nWidth < 255);  /* Simple control, no logical meaning */
  ASSERT(sFileName != NULL);
  ASSERT(sFileName[0] != '\0');  /* There should be a name */
  ASSERT(sTitle != NULL);
  ASSERT(nCopy >= 0);

  FSplit(sFileName, sPath, sFileNm, sAllMask, TRUE, TRUE);
  nFWidth = nWidth - (strlen(sFileNm) + 3);
  if (nFWidth < 12)
    nFWidth = 12;  /* a plausible minimal value */
  /* Remove trailing slash; This will force ShrinkPath to
  threat the last directory like it is a file name,
  and this is what we need to display in format c:\dir1\subdir2\...\actuadir */
  p = strrchr(sPath, PATH_SLASH_CHAR);
  ASSERT(p != NULL);
  if ((*(p - 1) != ':') && (p != sPath))  /* c:\ or / ? */
    *p = '\0';  /* if it is not root only, remove the trailing slash */
  ShrinkPath(sPath, sShrunkPath, nFWidth, TRUE);

  /* ShrinkPath() will add wildargs *.* when operating on directory path only */
  p = strchr(sShrunkPath, '*');  /* search for the first wildarg appearance */
  if (p != NULL)
    *p = '\0';  /* remove wildargs */
  if (sShrunkPath[0] != '\0')
    AddTrailingSlash(sShrunkPath);

  sCopy[0] = '\0';
  if (nCopy > 0)
    sprintf(sCopy, ":%d", nCopy + 1);

  strcpy(sTitle, sFileNm);
  strcat(sTitle, sCopy);
  if (sShrunkPath[0] != '\0')
  {
    strcat(sTitle, " (");
    strcat(sTitle, sShrunkPath);
    strcat(sTitle, ")");
  }

  if (sViewID != NULL)
    ShrinkPath(sPath, sViewID, 0, TRUE);
}

/* ************************************************************************
   Function: PrepareFileTitle
   Description:
     Prepares file title to be displayed at the status line.
     Prepares the viewID of the file.
*/
static void PrepareFileTitle(TFile *pFile, TFileList *pFileList)
{
  TView *pView;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(pFile->sFileName[0] != '\0');  /* There should be a name */

  pView = FileListFindFView(pFileList, pFile);
  ASSERT(pView != NULL);
  PrepareFileNameTitle(pFile->sFileName, pFile->nCopy, pFile->sTitle, 40,
    pView->sViewID);
}

typedef struct _ProcessFileContext
{
  TMRUList *pstProcessMRUList;  /* For ProcessFile() */
  TFileList *pstProcessFileList;  /* For ProcessFile() */
} TProcessFileContext;

/* ************************************************************************
   Function: ProcessFile
   Description:
     Call-back function passed as a parameter of RemoveFile() [filenav.c]
     Performs any update operations necessary due to changed
     file copy number (TFile.nCopy)
*/
static void ProcessFile(TFile *pFile, void *_pContext)
{
  TMRUListItem *pMRUEntry;
  TProcessFileContext *pstContext;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(_pContext != NULL);

  pstContext = (TProcessFileContext *)_pContext;

  /*
  Search for (nCopy + 1) as in the pMRUEntry is the old nCopy number
  for the particular file.
  */
  pMRUEntry = SearchInMRUList(pstContext->pstProcessMRUList, pFile->sFileName, pFile->nCopy + 1);
  if (pMRUEntry == NULL)
    return;

  pMRUEntry->nCopy = pFile->nCopy;
  PrepareFileTitle(pFile, pstContext->pstProcessFileList);
  if (pFile->nCopy == 0)  /* When becomes #0 copy, a recovery name is necessary */
    PrepareRecoveryFileName(pFile, pstContext->pstProcessFileList);
}

/* ************************************************************************
   Function: RemoveTopFile
   Description:
     Removes top file from pFileList.
     Processes the necessary changes in the MRU list by passing
     the ProcessFile() call-back function.

     The editor should always have at least one file. This function
     takes care to open a new empty file "noname" when all files are closed.
*/
BOOLEAN RemoveTopFile(TFileList *pFileList, TMRUList *pMRUList)
{
  TFile *pFile;
  TProcessFileContext stContext;

  stContext.pstProcessMRUList = pMRUList;  /* Parameter for ProcessFile() call-back func */
  stContext.pstProcessFileList = pFileList;  /* Parameter for ProcessFile() call-back func */
  pFile = RemoveFile(pFileList, ProcessFile, &stContext);
  DisposeFile(pFile);
  s_free(pFile);
  if (pFileList->nNumberOfFiles == 0)
  {
    pFile = AddNewFile(pFileList);
    strcpy(pFile->sMsg, sAllFilesClosed);
    return FALSE;
  }
  return TRUE;
}

/* ************************************************************************
   Function: RemoveLastFile
   Description:
     Same like RemoveTopFile() but no files are left behind.
     This function is to be called from DoneWorkspace().
*/
void RemoveLastFile(TFileList *pFileList, TMRUList *pMRUList)
{
  TFile *pFile;
  TProcessFileContext stContext;

  stContext.pstProcessMRUList = pMRUList;  /* Parameter for ProcessFile() call-back func */
  stContext.pstProcessFileList = pFileList;  /* Parameter for ProcessFile() call-back func */
  pFile = RemoveFile(pFileList, ProcessFile, &stContext);
  DisposeFile(pFile);
  s_free(pFile);
}

/* ************************************************************************
   Function: LoadAndApplyRecoveryFile
   Description:
     Checks for recovery file.
     Eixits if no such a file.
     Loads the file.
     Asks the user and if allowed applies the recovery file.
*/
static void LoadAndApplyRecoveryFile(TFile *pFile)
{
  BOOLEAN bApplyRecovery;
  BOOLEAN bStoreRecovery;  /* After applying partial recovery */

  ASSERT(VALID_PFILE(pFile));
  ASSERT(pFile->nCopy == 0);
  ASSERT(pFile->sRecoveryFileName[0] != '\0');

  /*
  Ask to apply a recovery file if exists.
  */
  if (!FileExists(pFile->sRecoveryFileName))
    return;
  if (ConsoleMessageProc(NULL, MSG_WARNING | MSG_YESNO, pFile->sFileName, sRecover) != 0)
    return;

  bApplyRecovery = FALSE;
  bStoreRecovery = FALSE;
  switch (LoadRecoveryFile(pFile))
  {
    case 0:
      bApplyRecovery = TRUE;
      break;
    case 1:  /* errno error */
      ConsoleMessageProc(NULL, MSG_ERRNO | MSG_ERROR | MSG_OK,
        pFile->sRecoveryFileName, NULL);
      break;
    case 2:  /* memory error -- can not load recovery file */
      ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK, NULL, sNoMemoryLRec);
      bNoMemory = FALSE;
      break;
    case 3:
      /* Recovery file corrupted. Partial recovery impossible */
      ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK,
        pFile->sRecoveryFileName, sRecoverCorrupted2);
      DisposeUndoIndexData(pFile);
      break;
    case 4:  /* Will apply partial recovery */
      ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK,
        pFile->sRecoveryFileName, sRecoverCorrupted1);
      bApplyRecovery = TRUE;
      bStoreRecovery = TRUE;  /* Store a valid recovery file after the recovery */
      break;
    case 5:  /* file in memory is not relevant to the recovery file */
      ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK,
        pFile->sRecoveryFileName, sRecoverInconsistent);
      break;
    default:
      ASSERT(0);  /* Invalid return value by LoadRecoveryFile() */
  }

  if (bApplyRecovery)
  {
    if (!RecoverFile(pFile))
    {
      ASSERT(bNoMemory);
      ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK, NULL, sNoMemoryRec);
      bNoMemory = FALSE;
    }
    else  /* rec file reflects changes in the file in memory -- indicate */
    {
      if (bStoreRecovery)  /* we need a valid recovery file */
      {
        pFile->bForceNewRecoveryFile = TRUE;  /* remove the corrupted file */
        pFile->bRecoveryStored = StoreRecoveryRecord(pFile);
      }
      else  /* we have a valid recovery file, keep adding records */
        pFile->bRecoveryStored = TRUE;
    }
  }

  if (pFile->nNumberOfRecords > 0)  /* If something recovered */
    pFile->bForceNewRecoveryFile = FALSE;  /* As we already have a recfile */
}

/* ************************************************************************
   Function: LoadFile
   Description:
     Calls LoadFilePrim() to load file from disk.
     Performs necessary manipulations concerning workspace components
     such as in-memory-files list and MRU files list.
     Applyes recovery file if exists.
   Parameters:
     sFileName -- file to load.
     pFileList -- workspace in-memory-files list.
     pMRUList -- workspace MRU list.
     bReload -- this flag concerns only the way the MRU list is maintained.
       When this flag is TRUE the correspondent MRU entry is not updated,
       if this flag is FALSE this file MRU entry is put on top of the MRU
       list.
     nForceReadOnly -- will set pFile->bForceReadOnly if the file is
       successfully loaded. If the file is present in the MRU list
       bForceReadOnly value will be copied from the MRU entry.
     MessageProc -- universal messaging functions.
   On exit:
     A new file is added on top of in-memory-files list (current file).
     Current cursor position matches the MRU list entry (if there's an entry).
     New entry is allocated if there's no MRU entry for this file.
   On failure:
     If there's no file with specified name a new empty file will be create
     in memory.
     If file fails to load pFileList remains unchaned.
     If recovery file failure occures only a message is displayed but the
     disk file remains in memory and pFileList is updated.
*/
void LoadFile(const char *sFileName, TFileList *pFileList, TMRUList *pMRUList,
  TDocType *pDocTypeSet, BOOLEAN bReload, BOOLEAN bForceReadOnly)
{
  TFile *pFile;
  TMRUListItem *pMRUEntry;
  char sPath[_MAX_PATH];
  char sFileNameOnly[_MAX_PATH];
  char sFullName[_MAX_PATH];
  TDocType *pDocType;

  /* Show a message that the file loading is in progress */
  ConsoleMessageProc(NULL, MSG_STATONLY | MSG_INFO, sFullName, sLoading);

  /*
  Produce the real file name in sFullName. FSplit() get the real file name
  (preserving letters case).
  */
  FSplit(sFileName, sPath, sFileNameOnly, sAllMask, TRUE, TRUE);
  strcpy(sFullName, sPath);
  strcat(sFullName, sFileNameOnly);

  pFile = AddFileInFileList(pFileList, sFullName);

  if (pFile->nCopy > 0)
  {
    /* TODO: Ask whether to load another copy in memory? */
  }

  /* Show a message that the file loading is in progress */
  ConsoleMessageProc(NULL, MSG_STATONLY | MSG_INFO, sFullName, sLoading2);

  switch (LoadFilePrim(pFile))
  {
    case 3:  /* No memory to load the file */
      RemoveTopFile(pFileList, pMRUList);
      return;
    case 4:  /* Invalid path -- errno */
    case 5:  /* Error while reading file -- errno */
      ConsoleMessageProc(NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, sFullName, NULL);
      return;
    case 2:  /* File with such a name doesn't exists */
      PrepareFileTitle(pFile, pFileList);
      PrepareRecoveryFileName(pFile, pFileList);
      strcpy(pFile->sMsg, sNewFileMsg);
      pFile->bNew = TRUE;
      pFile->nEOLType = DEFAULT_EOL_TYPE;  /* as no EOL type is assigned */
      if (pFile->nCopy > 0)
        goto _prepare_function_names_bookmarks;
      pFile->bForceNewRecoveryFile = TRUE;
      LoadAndApplyRecoveryFile(pFile);
      /*
      File is not added to MRU list as it may be not saved or changed at all.
      On exit bNew is checked in order to add the file to MRU list.
      */
      goto _prepare_function_names_bookmarks;
  }

  pDocType = DetectDocument(pDocTypeSet, pFile->sFileName);
  pFile->nType = tyPlain;
  if (pDocType != NULL)
    pFile->nType = pDocType->nType;

  if (pFile->nCopy > 0)
    pFile->bForceReadOnly = TRUE;

  if (!GetFileParameters(pFile->sFileName, NULL, &pFile->LastWriteTime,
    &pFile->nFileSize))
  {
    /* Above should always succeed on an existing file */
    ASSERT(0);
  }

  PrepareFileTitle(pFile, pFileList);
  if (pFile->nCopy == 0)
    PrepareRecoveryFileName(pFile, pFileList);

  /*
  Check whether such a file exists in the MRU list and
  get the file parameters from the MRU entry.
  */
  pMRUEntry = SearchInMRUList(pMRUList, pFile->sFileName, pFile->nCopy);
  if (pMRUEntry != NULL)
  {
    pFile->nTopLine = pMRUEntry->nTopLine;
    GotoColRow(pFile, pMRUEntry->nCol, pMRUEntry->nRow);
    pFile->bForceReadOnly = pMRUEntry->bForceReadOnly;
  }
  else
    GotoColRow(pFile, 0, 0);

  /*
  We have bForceReadOnly supplied as a paremeter of LoadFile(), on other
  hand we have (may have) a value set from the MRUEntry.
  Q: How to proceed?
  A: The bForceReadOnly flag should be extracted from the MRU entry only when
  LoadFile() is invoked from RestoreMRUFiles() and LoadMRUFile(). These
  functions call LoadFile() with the flag bReload set to TRUE. So in case
  of bReload = FALSE, bForceReadOnly should be overriden by the supplied
  LoadFile() parameter. We should check for the case of nCopy > 0 as
  then this flag has already been set.
  */
  if (!bReload && pFile->nCopy == 0)
    pFile->bForceReadOnly = bForceReadOnly;  /* Override */

  pFile->bUpdatePage = TRUE;

  /*
  Move the file entry at the top of the MRU list.
  */
  if (!bReload)
    AddFileToMRUList(pMRUList, pFile->sFileName, pFile->nCopy, FALSE, TRUE,
      pFile->bForceReadOnly, pFile->nCol, pFile->nRow, pFile->nTopLine, FALSE);

  /*
  Check	for recovery file.
  Process the recovery file.
  */
  if (pFile->nCopy == 0)
  {
    pFile->bForceNewRecoveryFile = TRUE;
    LoadAndApplyRecoveryFile(pFile);
  }

  /*
  Function names and locations are store in a bookmarks set
  for this file.
  */
_prepare_function_names_bookmarks:
  return;
}

/* ************************************************************************
   Function: CheckActivateFile
   Description:
     Checks whether the file exists in memory. If it is not, then loads the
     file. If the file is already loaded makes the file to be the active file.
*/
BOOLEAN CheckActivateFile(const char *sFileName, TFileList *pFileList, TMRUList *pMRUList)
{
  int nFileNumber;
  int nNumberOfFiles;
  TFile *pFile;
  int nLastFile;
  void OnFileSwitched(TFile *pLastFile);


  pFile = GetFileListTop(pFileList);  /* This is the current file */
  nLastFile = -1;
  if (pFile != NULL)
    nLastFile = pFile->nID;

  nFileNumber = SearchFileList(pFileList, sFileName, 0);
  if (nFileNumber == -1)  /* The file is not in memory */
  {
    nNumberOfFiles = pFileList->nNumberOfFiles;
    LoadFile(sFileName, pFileList, pMRUList, DocumentTypes, FALSE, FALSE);
    if (nNumberOfFiles == pFileList->nNumberOfFiles)
      return FALSE;  /* No files added after loading -- failure */
  }
  else
    SetTopFileByLoadNumber(pFileList, nFileNumber);

  pFile = GetFileListTop(pFileList);  /* This is the current file */
  if (nLastFile != pFile->nID)
    OnFileSwitched(SearchFileListByID(pFileList, nLastFile));
  return TRUE;
}

/* ************************************************************************
   Function: ResetOpened
   Description:
     A call-back function to be invoked for
     all the files in a MRUList. Sets all the bClosed flags in a MRUList.
*/
static void ResetOpened(TMRUListItem *pMRUItem, void *pContext)
{
  ASSERT(pMRUItem != NULL);

  pMRUItem->bClosed = TRUE;
}

/*
Bellow some parameters necessary for OpenMRUFile() call-back function.
Passed by RestoreMRUFiles() via global variables.
*/
static int nSaveCurrentFile;  /* The "current file" from the MRU list */
static TMRUList *_pMRUList;
static TFileList *_pFilesList;
static TDocType *_pDocTypeSet;

/* ************************************************************************
   Function: OpenMRUFile
   Description:
     A call-back function to be invoked for all the file in a MRUList.
     Opens all the files having bOpened flag set.
*/
static void OpenMRUFile(TMRUListItem *pMRUItem, void *pContext)
{
  ASSERT(pMRUItem != NULL);

  if (pMRUItem->bClosed)
    return;

  LoadFile(pMRUItem->sFileName, _pFilesList, _pMRUList,
    _pDocTypeSet, TRUE, FALSE);
  if (pMRUItem->bCurrent)
    nSaveCurrentFile = GetTopFileNumber(_pFilesList);
}

/* ************************************************************************
   Function: RestoreMRUFiles
   Description:
     Restores all the files that were opened from the last
     editor session. pMRUList should be a valid list that is
     loaded from the ini file.
     bRestoreLastFiles if FALSE will direct last opened
     files to be not reloaded at a new session. In this case
     all the bOpened flags in the MRUList should be reset
     so the list to remain valid.
*/
void RestoreMRUFiles(TFileList *pFilesList, TMRUList *pMRUList, TDocType *pDocTypeSet)
{
  if (bRestoreLastFiles)
  {
    /*
    Prepare the parameters for the OpenMRUFile() call-back function
    */
    _pMRUList = pMRUList;
    _pFilesList = pFilesList;
    _pDocTypeSet = pDocTypeSet;
    nSaveCurrentFile = -1;

    /*
    bUpward parameter should be FALSE, this will
    result in proper nCopy numbers of the restored files.
    */
    MRUListForEach(pMRUList, OpenMRUFile, FALSE, NULL);

    if (nSaveCurrentFile != -1)
      SetTopFileByLoadNumber(pFilesList, nSaveCurrentFile);
  }
  else
    MRUListForEach(pMRUList, ResetOpened, FALSE, NULL);
}

/* ************************************************************************
   Function: LoadMRUFile
   Description:
     This function is to be invoked when a file from 'File|Recent Files'
     menu is to be loaded.
     The file loaded only if is already not in memory.
*/
void LoadMRUFile(int nFile, TFileList *pFileList, TMRUList *pMRUList, TDocType *pDocTypeSet)
{
  TMRUListItem *pMRUItem;
  int nFileNumber;

  ASSERT(nFile >= 0);
  ASSERT(pFileList != NULL);
  ASSERT(pMRUList != NULL);

  pMRUItem = SearchInMRUListNumber(pMRUList, nFile);

  ASSERT(pMRUItem != NULL);

  nFileNumber = SearchFileList(pFileList, pMRUItem->sFileName, 0);
  if (nFileNumber != -1)
  {
    SetTopFileByLoadNumber(pFileList, nFileNumber);
    return;
  }

  LoadFile(pMRUItem->sFileName, pFileList, pMRUList, pDocTypeSet, FALSE, FALSE);
}

/* ************************************************************************
   Function: AddNewFile
   Description:
     Adds an empty new file to pFileList.
     Checks if there is a recovery file remaining from previous
     session for the new file.
*/
TFile *AddNewFile(TFileList *pFileList)
{
  TFile *pFile;

  ASSERT(pFileList != NULL);

  pFile = AddFileInFileList(pFileList, NULL);
  pFile->bNew = TRUE;
  pFile->nEOLType = DEFAULT_EOL_TYPE;
  PrepareFileTitle(pFile, pFileList);
  PrepareRecoveryFileName(pFile, pFileList);
  if (pFile->nCopy == 0)
  {
    pFile->bForceNewRecoveryFile = TRUE;
    LoadAndApplyRecoveryFile(pFile);
  }
  return pFile;
}

/* ************************************************************************
   Function: StoreFile
   Description:
     -- Calls StoreFilePrim() to store file to disk.
     -- Produces a backup file.
     -- If the file is new adds the file to the MRU list.
     -- If the store operation is successfull removes the recovery file.
     -- Updates the file parameters (size, data, tame, etc.) as stored at the disk.
     -- Updates the undo list to indicate that the point where the file
     is stored.
*/
void StoreFile(TFile *pFile, TMRUList *pMRUList)
{
  char sBuf[_MAX_PATH + 40];
  char sShrunkName[_MAX_PATH];
  char sTargetName[_MAX_PATH];
#ifdef UNIX
  char sTargetFileName[_MAX_PATH];
  char sTargetPath[_MAX_PATH];
  char sTargetNameTmp[_MAX_PATH];
  int nTempLen;
#endif
  char sBackupName[_MAX_PATH];
  char sOutput[25];
  int i;
  TUndoRecord *pUndoRec;
  int nOutputEOLType;

  if (pFile->bReadOnly || pFile->bForceReadOnly)
    return;  /* Store nothing when in read-only mode */

  /* Show a message that the file saving is in progress */
  ConsoleMessageProc(NULL, MSG_STATONLY | MSG_INFO, pFile->sFileName, sSaving);

  /*
  Produce .BAK file
  Don't produce .BAK if the file is new (no old version to preserve)
  Don't produce .BAK if the file is not changed (old .bak version is still valid)
  */
  if (bBackup && !pFile->bNew && pFile->bChanged)
  {
    strcpy(sTargetName, pFile->sFileName);
    /*
    Check to see whether the file is a link.
    */
#ifdef UNIX
_check_for_link:
    /* Obtain the name of the file where the link points at */
    nTempLen = readlink(sTargetName, sTargetNameTmp, sizeof(sTargetNameTmp) - 1);
    if (nTempLen != -1)
    {
      sTargetNameTmp[nTempLen] = '\0';
      /* TRACE2("link: %s -> %s\n", sTargetName, sTargetNameTmp); */
      /*
      TODO: As links are added to WIN2000, here is to be
      added code to check for full path in WIN32 'drive:\' specification.
      It would be best to make a function in path.c to do this
      like BOOLEAN HasRoot(const char *sFileName)
      */
      if (sTargetNameTmp[0] != '/')
      {
        /* the link points as a relative path */
        if (!FSplit(sTargetName, sTargetPath, sTargetFileName, "ERROR", FALSE, TRUE))
        {
          /* FSplit() failed: display error */
          ConsoleMessageProc(NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, sTargetName, NULL);
        }
        strcpy(sTargetName, sTargetPath);
        strcat(sTargetName, sTargetNameTmp);
      }
      else
      {
        /* the links points as a full path (has a root directory specified) */
        strcpy(sTargetName, sTargetNameTmp);
      }
      /* TRACE1("sTargetName: %s\n", sTargetName); */
      goto _check_for_link;
    }
#endif  /* ifdef UNIX */
    strcpy(sBackupName, sTargetName);
    ChangeFileNameExtention(sBackupName, sBak);
    /*
    Check whether the .bak file exists
    */
    if (FileExists(sBackupName))
    {
      /* .BAK file exists. Remove the .BAK file	*/
      if (unlink(sBackupName))
      {
        /* unlink failed: display error */
        ConsoleMessageProc(NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, sBackupName, NULL);
        return;
      }
    }
    if (rename(sTargetName, sBackupName))
    {
      ConsoleMessageProc(NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, pFile->sFileName, NULL);
      return;
    }
  }

  nOutputEOLType = nFileSaveMode == -1 ? pFile->nEOLType : nFileSaveMode;
  switch (StoreFilePrim(pFile, nOutputEOLType))
  {
    case 0:   /* Stored sucessfully */
      break;
    case 1:  /* Failed to open the file for writing */
      ConsoleMessageProc(NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, pFile->sFileName, NULL);
      return;
    case 2:  /* Failed to store whole the file */
      ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK, pFile->sFileName, sSaveFailed);
      return;
    case 3:  /* No memory to convert the file to be stored to disk */
      return;
    default:
      ASSERT(0);
  }

  /*
  If the INI file has been edited and store rise a flag
  to skip storing the current configuration on exit.
  */
  if (filestrcmp(pFile->sFileName, sINIFileName) == 0)
    bDontStoreINIFile = TRUE;
  if (filestrcmp(pFile->sFileName, sMasterINIFileName) == 0)
    bDontStoreMasterINIFile = TRUE;

  /*
  Prepare a message at the status line
  that the file was successfully stored.
  */
  ShrinkPath(pFile->sFileName, sShrunkName, CurWidth - 25, FALSE);
  sprintf(sBuf, sFileSaved, sShrunkName);
  strcpy(pFile->sMsg, sBuf);

  /*
  Indicate whether the file has been converted.
  */
  sOutput[0] = '\0';
  if (nFileSaveMode != -1)  /* If mode is not "auto" */
    if (pFile->nEOLType != nFileSaveMode)  /* Change of EOL type? */
      if (pFile->nEOLTypeDisk != nFileSaveMode)
        sprintf(sOutput, sConverted, sEOLTypes[nFileSaveMode + 1]);
  strcat(pFile->sMsg, sOutput);
  pFile->nEOLTypeDisk = nOutputEOLType;  /* File format as stored on disk */

  pFile->bChanged = FALSE;
  if (pFile->bNew)
  {
    /* As the file exists on disk it is time to put the file in MRU list */
    AddFileToMRUList(pMRUList, pFile->sFileName, pFile->nCopy, FALSE, TRUE,
      pFile->bForceReadOnly, pFile->nCol, pFile->nRow, pFile->nTopLine, FALSE);
    pFile->bNew = FALSE;
  }

  /*
  Remove the recovery file.
  */
  if (FileExists(pFile->sRecoveryFileName))
  {
    /* .rec file exists. Remove the .rec file */
    if (unlink(pFile->sRecoveryFileName) != 0)
    {
      /* unlink failed: display error */
      ConsoleMessageProc(NULL, MSG_ERRNO | MSG_ERROR | MSG_OK,
        pFile->sRecoveryFileName, NULL);
    }
    pFile->bRecoveryStored = FALSE;
    pFile->bForceNewRecoveryFile = TRUE;
  }

  /*
  Get the file parameters as store on the disk.
  NOTE: GetFileParamaters() is supposed to succeed always
  on a file that was just written sucessfully.
  */
  if (!GetFileParameters(pFile->sFileName, NULL, &pFile->LastWriteTime,
    &pFile->nFileSize))
  {
    ASSERT(0);
  }

  /*
  Prepare undo list pRecoveryStore flag to reflect the fact
  that the file has just being stored.

  Set bRecoveryStored flag to FALSE for the undone blocks.
  */
  for (i = pFile->nNumberOfRecords - 1; i >= 0; --i)
  {
    pUndoRec = &pFile->pUndoIndex[i];
    ASSERT(VALID_PUNDOREC(pUndoRec));
    ASSERT(pUndoRec->pData != NULL);
    if (pUndoRec->bUndone)
      pUndoRec->bRecoveryStore = FALSE;
    else
      break;
  }

  /*
  Set bRecoveryStored up to the bottom of the undo list
  as for these items the disk file has stored the info.
  */
  for (; i >= 0; --i)
  {
    pUndoRec = &pFile->pUndoIndex[i];
    ASSERT(VALID_PUNDOREC(pUndoRec));
    ASSERT(pUndoRec->pData != NULL);
    ASSERT(!pUndoRec->bUndone);
    pUndoRec->bRecoveryStore = TRUE;
  }

  pFile->bForceNewRecoveryFile = TRUE;
}

/* ************************************************************************
   Function: StoreFile
   Description:
     Changes the file name to sFileName;
     Calls StoreFile() to save the file on disk;
*/
void StoreFileAs(char *sFileName, TFile *pFile, TFileList *pFileList,
  TMRUList *pMRUList, TDocType *pDocTypeSet)
{
  TDocType *pDocType;

  ASSERT(sFileName != NULL);
  ASSERT(sFileName[0] != '\0');
  ASSERT(VALID_PFILE(pFile));

  /*
  Mark the old file as closed in the MRU list
  */
  AddFileToMRUList(pMRUList, pFile->sFileName, pFile->nCopy, TRUE, FALSE,
    pFile->bForceReadOnly, pFile->nCol, pFile->nRow, pFile->nTopLine, TRUE);

  /*
  Remove the recovery file if exists
  */
  if (FileExists(pFile->sRecoveryFileName))
  {
    /* .rec file exists. Remove the .rec file */
    if (unlink(pFile->sRecoveryFileName) != 0)
    {
      /* unlink failed: display error */
      ConsoleMessageProc(NULL, MSG_ERRNO | MSG_ERROR | MSG_OK,
        pFile->sRecoveryFileName, NULL);
    }
    pFile->bRecoveryStored = FALSE;
    pFile->bForceNewRecoveryFile = TRUE;
  }

  /*
  Set the new name
  */
  strcpy(pFile->sFileName, sFileName);
  PrepareFileTitle(pFile, pFileList);
  if (pFile->nCopy == 0)
    PrepareRecoveryFileName(pFile, pFileList);

  /*
  Put the file with the new name in the MRU list
  */
  AddFileToMRUList(pMRUList, pFile->sFileName, pFile->nCopy, FALSE, TRUE,
    pFile->bForceReadOnly, pFile->nCol, pFile->nRow, pFile->nTopLine, FALSE);

  StoreFile(pFile, pMRUList);

  /*
  Document type might have changed
  */
  pDocType = DetectDocument(pDocTypeSet, pFile->sFileName);
  pFile->nType = tyPlain;
  if (pDocType != NULL)
    pFile->nType = pDocType->nType;
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

