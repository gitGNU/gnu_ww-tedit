/*

File: searchf.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 25th April, 2000
Descrition:
  Functions concerning text search and replace in multiple files.

*/

#include "global.h"
#include "disp.h"
#include "l1def.h"
#include "palette.h"
#include "l2disp.h"
#include "file.h"
#include "nav.h"
#include "findf.h"
#include "search.h"
#include "smalledt.h"
#include "keyset.h"
#include "defs.h"
#include "wrkspace.h"
#include "memory.h"
#include "filenav.h"
#include "searchf.h"

TBookmarksSet *pstFindInFiles;  /* Could be stFindInFiles1 or stFindInFiles2 */
static char sDirectories[_MAX_PATH];  /* Comma separated directories and +files */
static char sMasks[_MAX_PATH];  /* Comma separated masks */
static char sPattern[_MAX_PATH];  /* Search for this */
static BOOLEAN bRecursive;
static BOOLEAN bIgnoreCase;
static BOOLEAN bWindow2;
static BOOLEAN bRegularExpr;
static TSearchContext stSearchContextFiles;

/* ************************************************************************
   Function: DisplayOutput
   Description:
     This function is called whenever new entry is added to pstFindInFiles.
     Calls DisplayBookmarksFunc() and then updates the Viewer on
     the screen.
*/
static BOOLEAN DisplayOutput(TMarkLocation *pMark, int nRow)
{
  TDisplayBookmCtx stCtx;
  disp_event_t ev;

  stCtx.pThisSet = pstFindInFiles;
  stCtx.nPreferredLine = 0;
  if (!DisplayBookmarksFunc(pMark, nRow, &stCtx))
    return FALSE;
  pstFindInFiles->bViewDirty = FALSE;
  BookmarksInvalidate(pstFindInFiles);

  disp_event_clear(&ev);
  ev.t.code = EVENT_USR;
  ev.t.user_msg_code = MSG_UPDATE_SCR;
  ev.data1 = 0;  /* TODO: wrkspace here! */
  ContainerHandleEvent(pstFindInFiles->stView.pContainer, &ev);
  return TRUE;
}

/* ************************************************************************
   Function: MatchFile
   Description:
     Try to match a file against a list of masks
*/
BOOLEAN MatchFile(const char *psFile, const char *psMasks)
{
  char sTempBuf[_MAX_PATH];
  char *pMask;
  char *pLast;

  strcpy(sTempBuf, psMasks);  /* as strtok() will destroy the string */

  /*
  Try to match the filename against a mask
  in the list of masks (separated by ';')
  WE CANNOT USE strtok() HERE, because FindInFiles()
  uses strtok().
  */
  pMask = strchr(sTempBuf, ';');
  pLast = sTempBuf;
  while (pLast != NULL)
  {
    if (pMask != NULL)
      *pMask = '\0';
    if (match_wildarg(psFile, pLast))
      return TRUE;
    if (pMask == NULL)
      return FALSE;
    ++pMask;
    if (*pMask == '\0')
      return FALSE;
    pLast = pMask;
    pMask = strchr(pMask, ';');
  }
  return FALSE;
}

/* ************************************************************************
   Function: Global
   Description:
     For all the files in sPath calls pfnProcessFile().
     sPath is in format dir\*mask1;*mask2;*mask3
*/
static BOOLEAN Global(dispc_t *disp, const char *sPath,
  BOOLEAN (*pfnProcessFile)(dispc_t *disp, const char *psFile), int nLevel,
  BOOLEAN (*pfnShowDir)(dispc_t *disp, const char *psDir))
{
  char sPathOnly[_MAX_PATH];
  char sMasks[_MAX_PATH];  /* All the masks separated by ";" */
  char sReadDirMask[_MAX_PATH];
  char sBuf[_MAX_PATH];
  struct FF_DAT *ff_dat;
  struct findfilestruct ff;

  ASSERT(sPath != NULL);
  ASSERT(pfnProcessFile != NULL);

  if (nLevel == 64)
    return TRUE;

  /* Separate path and filename */
  if (!FSplit(sPath, sPathOnly, sMasks, sDirMask, TRUE, FALSE))
    return FALSE;

  /*
  Read the directories first
  */
  strcpy(sReadDirMask, sPathOnly);
  if (pfnShowDir != NULL)
    if (!pfnShowDir(disp, sReadDirMask))
      return FALSE;
  strcat(sReadDirMask, sAllMask);  /* "*.*" */

  ff_dat = find_open(sReadDirMask, FFIND_DIRS);
  if (ff_dat == NULL)
    return TRUE;

  while (find_file(ff_dat, &ff) == 0)
  {
    if (strcmp(ff.filename, ".") == 0)
      continue;  /* Skip "." directory */
    if (strcmp(ff.filename, "..") == 0)
      continue;  /* Skip "." directory */

    strcpy(sBuf, sPathOnly);
    strcat(sBuf, ff.filename);  /* Full path for this entry */
    AddTrailingSlash(sBuf);
    strcat(sBuf, sMasks);
    if (bRecursive)
      if (!Global(disp, sBuf, pfnProcessFile, nLevel + 1, pfnShowDir))
        return FALSE;
  }

  find_close(ff_dat);

  ff_dat = find_open(sReadDirMask, FFIND_FILES);
  if (ff_dat == NULL)
    return TRUE;

  while (find_file(ff_dat, &ff) == 0)
  {
    if (!MatchFile(ff.filename, sMasks))
      continue;
    strcpy(sBuf, sPathOnly);
    strcat(sBuf, ff.filename);
    if (!(pfnProcessFile(disp, sBuf)))
    {
      /* ESC has been pressed to cancel */
      find_close(ff_dat);
      return FALSE;
    }
  }

  find_close(ff_dat);
  return TRUE;
}

static TMarkLocation *pstPrevMark;
static int nFileNumber;
static TFile *_pFile;

/* ************************************************************************
   Function: GetAFile
     a call-back function that extracts file with particular number
   TODO: CONTEXT
*/
static BOOLEAN GetAFile(TFile *pFile, void *pContext)
{
  if (nFileNumber == 0)
  {
    _pFile = pFile;
    return FALSE;
  }
  --nFileNumber;
  return TRUE;
}

/* ************************************************************************
   Function: ProcessFile
   Description:
     Load file. Search for the pattern. Generate bookmarks for the occurrences
     and unload the file.
*/
static BOOLEAN ProcessFile(dispc_t *disp, const char *psFile)
{
  TMarkLocation *pstMark;
  char *psContent;
  char sErrorMsg[_MAX_PATH];
  char sShortPath[_MAX_PATH];
  TFile File;
  BOOLEAN bLoad;
  int nOldCol;
  int nOldRow;
  int nOrigCol;
  int nOrigRow;
  int i;
  disp_wnd_param_t wnd_param;

  nFileNumber = SearchFileList(pFilesInMemoryList, psFile, 0);
  if (nFileNumber != -1)
  {
    _pFile = NULL;
    FileListForEach(pFilesInMemoryList, GetAFile, TRUE, NULL);
    ASSERT(_pFile != NULL);
    bLoad = FALSE;
    nOrigCol = _pFile->nCol;
    nOrigRow = _pFile->nRow;
    GotoColRow(_pFile, 0, 0);
  }
  else
  {
    InitEmptyFile(&File);
    strcpy(File.sFileName, psFile);
    switch (LoadFilePrim(&File))
    {
      case 0:  /* Load OK */
        break;
      case 2:  /* File doesn't exists */
        goto _dispose;
      case 3:  /* No memory */
        strcpy(sErrorMsg, sNoMemoryForFile);
_err_msg:
        ShrinkPath(File.sFileName, sShortPath, 0, TRUE);
        strcat(sErrorMsg, sShortPath);
        BMListInsert(NULL, 1, 0, -1, sErrorMsg, NULL, 0,
          pstFindInFiles, &pstMark, BOOKM_STATIC);
        DisplayOutput(pstMark, 0);
        if (pstPrevMark != NULL)
        {
          pstPrevMark->pNext = pstMark;
        }
        pstPrevMark = pstMark;
        goto _dispose;
      case 4:  /* Invalid path */
        /* TODO: generate a message */
        goto _dispose;
      case 5:  /* I/O error */
        strcpy(sErrorMsg, sUnableToOpen);
        goto _err_msg;
      default:  /* Invalid LoadFilePrim() output */
        ASSERT(0);
        goto _dispose;
    }
    _pFile = &File;
    GotoColRow(_pFile, 0, 0);
    bLoad = TRUE;
  }

  while (1)
  {
    nOldCol = _pFile->nCol;
    nOldRow = _pFile->nRow;
    disp_wnd_get_param(disp, &wnd_param);
    if (!Find(_pFile, 1, wnd_param.width, &stSearchContextFiles))
      break;

    for (i = 0; i <= stSearchContextFiles.nNumLines; ++i)
    {
      psContent = GetLineText(_pFile, _pFile->nRow + i);
      BMListInsert(_pFile->sFileName, _pFile->nRow + i, _pFile->nCol, -1,
        psContent, NULL, 0, pstFindInFiles, &pstMark, BOOKM_STATIC);
      DisplayOutput(pstMark, _pFile->nRow);
      if (pstPrevMark != NULL)
      {
        pstPrevMark->pNext = pstMark;
      }
      pstPrevMark = pstMark;
    }
  }

_dispose:
  if (bLoad)
    DisposeFile(&File);
  else
    GotoColRow(_pFile, nOrigCol, nOrigRow);

  /*
  TODO: disp, implement some kbhit replacement or go async
  if (kbhit(&nDummy))
    if (ReadKey() == KEY(0, kbEsc))
      return FALSE;
  */
  return TRUE;
}

/* ************************************************************************
   Function: SeparateComponents
   Description:
     Separate options, patterns, directories and masks.
     Parses the options.
     -s - subdirectories
     -i - ignore case
     -r - regular expression
     -2 - put results in FindInFiles2
*/
static void SeparateComponents(const char *sText)
{
  const char *pos;
  char *p;
  char sBuf[_MAX_PATH];
  char sTemp[_MAX_PATH];
  BOOLEAN bFirstWord;  /* First word is the search pattern */
  struct stat statbuf;
  char *pLastSlash;
  char *pLastColon;
  char *pMaskPos;
  BOOLEAN bQuoted;

  ASSERT(strlen(sText) < _MAX_PATH);
  strcpy(sBuf, sText);
  bFirstWord = TRUE;
  bRecursive = FALSE;
  bIgnoreCase = TRUE;
  bWindow2 = FALSE;
  bRegularExpr = FALSE;
  sDirectories[0] = '\0';
  sMasks[0] = '\0';
  sPattern[0] = '\0';

  pos = ExtractComponent(sText, sBuf, &bQuoted);
  p = sBuf;
  while (1)
  {
    if (!bQuoted)
    {
      if (*p == '-')
      {
        if (strncmp(p, "-s", 2) == 0)
        {
  _recursive:
          if (*(p + 2) == '\0' || *(p + 2) == '+')
            bRecursive = TRUE;
          else
            bRecursive = FALSE;
        }
        else
          if (strncmp(p, "-d", 2) == 0)
            goto _recursive;
          else
            if (strncmp(p, "-i", 2) == 0)
            {
              if (*(p + 2) == '\0' || *(p + 2) == '+')
                bIgnoreCase = TRUE;
              else
                bIgnoreCase = FALSE;
            }
            else
              if (strncmp(p, "-2", 2) == 0)
              {
                if (*(p + 2) == '\0' || *(p + 2) == '+')
                  bWindow2 = TRUE;
                else
                  bWindow2 = FALSE;
              }
              else
                if (strncmp(p, "-r", 2) == 0)
                {
                  if (*(p + 2) == '\0' || *(p + 2) == '+')
                    bRegularExpr = TRUE;
                  else
                    bRegularExpr = FALSE;
                }
                else
                  ;  /* TODO: error */
        goto _next_token;
      }  /* end of check for option character */
    }  /* end of not quoted */
    if (bFirstWord)
    {
      strcpy(sPattern, p);
      bFirstWord = FALSE;
    }
    else
    {
      /*
      Check whether it is a directory or a file or a mask
      */
      strcpy(sTemp, p);
      if (strlen(sTemp) > 1)
      {
        pLastSlash = strchr(sTemp, '\0');
        --pLastSlash;
        if (*pLastSlash == '\\' || *pLastSlash == '/')
          if (*(pLastSlash - 1) != ':')  /* case of c:\ */
          *pLastSlash = '\0';
      }
      if (stat(sTemp, &statbuf) != 0)
      {
        /* error extracting file attributes
        check whether this is a mask */
        if (HasWild(sTemp))
        {
          /* We need to check the case of
          directory/mask (directory and mask specified) */
          pLastSlash = strrchr(sTemp, PATH_SLASH_CHAR);
          pLastColon = strrchr(sTemp, ':');
          pMaskPos = pLastColon;
          if (pMaskPos == NULL)
            pMaskPos = pLastSlash;
          if (pMaskPos != NULL)
            goto _add_dir;
          if (sMasks[0] != '\0')
            strcat(sMasks, ";");
          strcat(sMasks, sTemp);
          goto _next_token;
        }
        /* assume it is a file */
        goto _add_file;
      }

      if ((statbuf.st_mode & S_IFDIR) != 0)
      {
        /* this is a directory */
_add_dir:
        if (sDirectories[0] != '\0')
          strcat(sDirectories, ",");
        strcat(sDirectories, sTemp);
      }
      else
      {
_add_file:
        if (sDirectories[0] != '\0')
          strcat(sDirectories, ",");
        strcat(sDirectories, "+");
        strcat(sDirectories, sTemp);
      }
    }
_next_token:
    if (pos == NULL)
      return;
    pos = ExtractComponent(pos, sBuf, &bQuoted);
    p = sBuf;
    if (*p == '\0')
      return;
  }
}

/* ************************************************************************
   Function: ShowDir
   Description:
     A call back function that displays the currently processed directory
*/
static BOOLEAN ShowDir(dispc_t *disp, const char *psDir)
{
  char sPath[_MAX_PATH];
  char sBuf[_MAX_PATH];
  disp_wnd_param_t wnd_param;

  disp_wnd_get_param(disp, &wnd_param);
  ShrinkPath(psDir, sPath, wnd_param.width - 4, FALSE);
  sprintf(sBuf, "%s...", sPath);
  DisplayStatusStr(sBuf, coStatusTxt, TRUE, TRUE, disp);
  /* TODO: disp, implement kbhit replacement or go async */
  /*
  if (kbhit(&nDummy))
    if (ReadKey() == KEY(0, kbEsc))
      return FALSE;
  */
  return TRUE;
}

extern void CmdWindowFindInFiles1(void *pCtx);
extern void CmdWindowFindInFiles2(void *pCtx);

/* ************************************************************************
   Function: FindInFiles
   Description:
*/
void FindInFiles(dispc_t *disp, const char *sText, int *pnWindow)
{
  char *p;
  char sBuf[_MAX_PATH];
  char sADir[_MAX_PATH];
  TMarkLocation *pstMark;
  BOOLEAN bMaskSpecified;
  BOOLEAN bFilesOnly;
  disp_event_t ev;

  ASSERT(sText != NULL);
  ASSERT(pnWindow != NULL);

  InitSearchContext(&stSearchContextFiles);
#ifdef WIN32
  /* we need to temporarily gear the priority down to normal
  as this blocks the computer for long periods of time */
  /*SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);*/
#endif

  /* TODO: display "preparing..." as directory components
  processing may take time */
  SeparateComponents(sText);

  *pnWindow = 1;
  pstFindInFiles = &stFindInFiles1;
  if (bWindow2)
  {
    pstFindInFiles = &stFindInFiles2;
    *pnWindow = 2;
  }

  BMListDisposeBMSet(pstFindInFiles);

  /*
  First apparition on the screen
  */
  if (bWindow2)
    CmdWindowFindInFiles2(NULL);
  else
    CmdWindowFindInFiles1(NULL);

  pstPrevMark = NULL;
  sprintf(sBuf, " Find in files: %s", sText);
  BMSetNewMsg(pstFindInFiles, (char *)sSearchInProgress);
  BMListInsert(NULL, 0, 0, -1, sBuf, NULL, 0,
    pstFindInFiles, &pstPrevMark, BOOKM_STATIC);
  DisplayOutput(pstPrevMark, 0);
  /* update entire scr for one last time, this will display the separator line */
  disp_event_clear(&ev);
  ev.t.code = EVENT_USR;
  ev.t.user_msg_code = MSG_UPDATE_SCR;
  ev.data1 = 0;  /* TODO: wrkspace here! */
  ContainerHandleEvent(&stRootContainer, &ev);

  /* Prepare search context data */
  stSearchContextFiles.bCaseSensitive = !bIgnoreCase;
  stSearchContextFiles.bRegularExpr = bRegularExpr;
  if (ParseSearchPattern(sPattern, &stSearchContextFiles) != 0)
  {
    BMListInsert(NULL, 1, 0, -1, "Error:", NULL, 0,
      pstFindInFiles, &pstMark, BOOKM_STATIC);
    if (pstPrevMark != NULL)
    {
      pstPrevMark->pNext = pstMark;
    }
    pstPrevMark = pstMark;
    BMListInsert(NULL, 1, 0, -1, stSearchContextFiles.sError, NULL, 0,
      pstFindInFiles, &pstMark, BOOKM_STATIC);
    DisplayOutput(pstMark, 0);
    if (pstPrevMark != NULL)
    {
      pstPrevMark->pNext = pstMark;
    }
    pstPrevMark = pstMark;
    goto _done_search;
  }

  /*
  In sDirectories we have directories separated by ",". Some of the directories
  may have supplied masks (one mask or group of masks separated by ";").
  Call global for all the directories. If any of the directories has no
  masks supplied, concatenate with sMasks (where we have masks specified
  separated by ";").
  sDirectories components that contain "+" as a prefix are filenames.
  */
  if (sDirectories[0] == '\0')
    strcpy(sDirectories, ".");
  bMaskSpecified = TRUE;
  if (sMasks[0] == '\0')
  {
    strcpy(sMasks, sAllMask);
    bMaskSpecified = FALSE;
  }
  strcpy(sBuf, sDirectories);
  p = strtok(sBuf, ",");
  if (bRecursive)
  {
    /* The idea is that when recursive search
    is specified to use the filenames as masks
    instead as single filenames */
    bFilesOnly = TRUE;
    while (p != NULL)
    {
      if (*p == '+' && (strchr(p + 1, PATH_SLASH_CHAR) == NULL))
      {
        if (bMaskSpecified)
        {
          strcat(sMasks, ";");
          strcat(sMasks, p + 1);
        }
        else
        {
          strcpy(sMasks, p + 1);
          bMaskSpecified = TRUE;
        }
      }
      else
        bFilesOnly = FALSE;
      p = strtok(NULL, ",");
    }
    strcpy(sBuf, sDirectories);
    if (bFilesOnly)
      strcat(sBuf, ",.");
    p = strtok(sBuf, ",");
  }
  while (1)
  {
    if (p == NULL)
      break;
    strcpy(sADir, p);
    if (sADir[0] == '+')
    {
      /* this is a file */
      if (!bRecursive)  /* the file was moved in sMasks */
        ProcessFile(disp, &sADir[1]);
      goto _next_token;
    }
    if (!HasWild(sADir))
    {
      AddTrailingSlash(sADir);
      strcat(sADir, sMasks);
    }
    if (!Global(disp, sADir, ProcessFile, 0, ShowDir))
    {
      /* here the row is 1, to maintain the sorted order of the messages
      as we need the sText message to be before "Canceled" */
      BMListInsert(NULL, 1, 0, -1, sCanceled, NULL, 0,
        pstFindInFiles, &pstMark, BOOKM_STATIC);
      if (pstPrevMark != NULL)
      {
        pstPrevMark->pNext = pstMark;
      }
      pstPrevMark = pstMark;
      break;
    }
_next_token:
    p = strtok(NULL, ",");
  }
_done_search:
  BMSetNewMsg(pstFindInFiles, NULL);
  DoneSearchContext(&stSearchContextFiles);
#ifdef WIN32
  /*SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);*/
#endif
}

/* ************************************************************************
   Function: PrepareFunctionNamesBookmarks
   Description:
*/
void PrepareFunctionNamesBookmarks(TFile *pFile)
{
  TFunctionName stFunctions[64];
  int nNumFunctions;
  int nCurLine;
  int nCurPos;
  int i;
  TBookmarksSet *pstFuncNames;
  char *psContent;
  int nRow;
  int nCol;
  char sBuf[_MAX_PATH + 80];
  char sShortName[_MAX_PATH];

  nCurLine = 0;
  nCurPos = 0;

  pstFuncNames = GetFuncNamesBMSet(pFilesInMemoryList, pFile);
  BMListDisposeBMSet(pstFuncNames);

  ShrinkPath(pFile->sFileName, sShortName, 0, TRUE);
  snprintf(sBuf, sizeof(sBuf), " [ %s ]", sShortName);
  BMListInsert(NULL, 0, 0, -1, sBuf, NULL, 0,
    pstFuncNames, NULL, BOOKM_STATIC);

_scan:
  nNumFunctions = FunctionNameScan(pFile, nCurLine, nCurPos,
    pFile->nNumberOfLines, 0,
    stFunctions, _countof(stFunctions));
  if (nNumFunctions == 0)
    return;

  for (i = 0; i < nNumFunctions; ++i)
  {
    nRow = stFunctions[i].nLine;
    nCol = stFunctions[i].nNamePos;
    psContent = GetLineText(pFile, nRow);
    BMListInsert(pFile->sFileName, nRow, nCol, -1,
      psContent, &stFunctions[i], sizeof(TFunctionName),
      pstFuncNames, NULL, BOOKM_STATIC);
#if 0
    {
    char sFunction[128];
    char *sLine;
    int nNameLen;
    memset(sFunction, 0, sizeof(sFunction));
    sLine = GetLineText(pFile, stFunctions[i].nLine);
    nNameLen = stFunctions[i].nNameLen;
    if (nNameLen > sizeof(sFunction) - 1)
      nNameLen = sizeof(sFunction) - 1;
    strncpy(sFunction,
      sLine + stFunctions[i].nNamePos, nNameLen);
    TRACE1("extract functions: %s\n", sFunction);
    }
#endif
  }

  nCurLine = stFunctions[i - 1].nFuncEndLine;
  nCurPos = stFunctions[i - 1].nFuncEndPos;
  goto _scan;
}

/* ************************************************************************
   Function: ApplyFuncBookmarksColors
   Description:
     To be used within TExtraColorInterf to highlight function names
     in the Search|Functions bookmarks viewer.
*/
void ApplyFuncBookmarksColors(const TFile *pFile, int bNewPage, int nLine,
  int nTopLine, int nWinHeight,
  TSynHInterf *pSynhInterf, struct ExtraColorInterf *pCtx)
{
  TBookmarksSet *pBMSetFuncNames;
  TBMFindNextCtx *pBMFindCtx;
  int nRow;
  TFunctionName *pFuncName;
  TMarkLocation *pMark;

  pBMSetFuncNames = pCtx->pReserved1;
  pBMFindCtx = pCtx->pExtraCtx;
  if (bNewPage)
  {
    pCtx->pReserved2 =
      BMSetFindFirstBookmark(pBMSetFuncNames, NULL, 0, nLine, pBMFindCtx, &nRow);
    pCtx->nReserved1 = nLine;
    return;
  }

  BMSetRevertToFirst(pBMFindCtx);  /* not very expensive */
  BMSetNewSearchMark(pBMFindCtx, nLine);
  pMark = BMSetFindNextBookmark(pBMFindCtx, &nRow);
  ASSERT(VALID_PMARKLOCATION(pMark));

  pFuncName = pMark->pExtraRec;
  if (pFuncName != NULL)
  {
    pSynhInterf->pfnPutAttr(COLOR_SFR,
      pFuncName->nNamePos + pMark->nRenderIndent,
      pFuncName->nNamePos + pMark->nRenderIndent + pFuncName->nNameLen - 1,
      pSynhInterf);
  }
}

/* ************************************************************************
   Function: RescanPageForFuncNames
   Description:
     Rescans a page for new function names or changes in existing names.
   bFirstTime: FALSE -> there is a cached anchor for BMSetRevertToFirst() to use
   bFirstTime: TRUE -> no cache, no marks, insert the function names.
   Returns:
     TRUE - bookmaks were inserted or deleted
     FALSE - no changes in the bookmarks list
*/
static BOOLEAN
RescanPageForFuncNames(const TFile *pFile,
  int nStartLine, int nEndLine, BOOLEAN bFirstTime)
{
  TFunctionName stFunctions[64];
  int nNumFunctions;
  int nCurLine;
  int nCurPos;
  int i;
  /*
  char sFunction[128];
  char *sLine;
  int nNameLen;
  */
  TBookmarksSet *pstFuncNames;
  char *psContent;
  int nRow;
  int nCol;
  TFunctionName *pFuncName;
  int nCalcRow;
  TMarkLocation *pMark;
  BOOLEAN bNewMarks;
  BOOLEAN bNewMarksGlobal;
  BOOLEAN bUpdate;
  TMarkLocation *pMarkToRemove;

  nCurLine = nStartLine;
  nCurPos = 0;
  bNewMarks = FALSE;
  bNewMarksGlobal = FALSE;
  bUpdate = FALSE;
  pMarkToRemove = NULL;

  pstFuncNames = pFile->pBMSetFuncNames;

_scan:
  nNumFunctions = FunctionNameScan(pFile, nCurLine, nCurPos,
    pFile->nNumberOfLines, nEndLine,
    stFunctions, _countof(stFunctions));

  /*
  First pass:
  Check for corrections of function names or adding new function names
  */
  for (i = 0; i < nNumFunctions; ++i)
  {
    nRow = stFunctions[i].nLine;
    nCol = stFunctions[i].nNamePos;
    psContent = GetLineText(pFile, nRow);

    if (bFirstTime)
    {
      bNewMarks = TRUE;
      goto _insert_func_name;
    }
    BMSetRevertToFirst(pFile->pBMFuncFindCtx);  /* not very expensive */
    BMSetNewSearchRow(pFile->pBMFuncFindCtx, nRow);

_next_bookmark:
    pMark = BMSetFindNextBookmark(pFile->pBMFuncFindCtx, &nCalcRow);

    /*
    Filter out the not BMSetFuncNames bookmarks
    */
    if (pMark != NULL)
      if (pMark->pSet != pFile->pBMSetFuncNames)
        goto _next_bookmark;

    if (pMark == NULL)
    {
      bNewMarks = TRUE;
      goto _insert_func_name;  /* No bookmarks */
    }
    if (nCalcRow != nRow)
    {
      bNewMarks = TRUE;
      goto _insert_func_name;  /* No function name bookmark for this specific line */
    }
    pFuncName = pMark->pExtraRec;
    if (    (pFuncName->nLine == stFunctions[i].nLine)
         && (pFuncName->nNamePos == stFunctions[i].nNamePos)
         && (pFuncName->nNameLen == stFunctions[i].nNameLen) )
      continue;  /* Same function name */
    BMListRemoveBookmark(pMark);  /* remove and reinsert */
    //TRACE1("remove: %s\n", pMark->psContent);

_insert_func_name:
    BMListInsert(pFile->sFileName, nRow, nCol, -1,
      psContent, &stFunctions[i], sizeof(TFunctionName),
      pstFuncNames, NULL, BOOKM_STATIC);
    bUpdate = TRUE;
    /*
    memset(sFunction, 0, sizeof(sFunction));
    sLine = GetLineText(pFile, stFunctions[i].nLine);
    nNameLen = stFunctions[i].nNameLen;
    if (nNameLen > sizeof(sFunction) - 1)
      nNameLen = sizeof(sFunction) - 1;
    strncpy(sFunction,
      sLine + stFunctions[i].nNamePos, nNameLen);
    PrintString("%s\n", sFunction);
    */
  }

  if (bNewMarks || bFirstTime)  /* can we use the cached context? */
  {
    /* Find the first bookmark of this page (nStartLine) */
    pMark = BMSetFindFirstBookmark(NULL,
      pFile->sFileName, nStartLine, 0, pFile->pBMFuncFindCtx, &nRow);
    if (pMark == NULL)  /* no bookmarks? */
      goto _prepare_next_extraction;
    bNewMarksGlobal = TRUE;
    bNewMarks = FALSE;
  }

  /*
  Second pass:
  Remove names that are no longer valid
  */
  BMSetRevertToFirst(pFile->pBMFuncFindCtx);  /* not very expensive */
  BMSetNewSearchRow(pFile->pBMFuncFindCtx, nStartLine);

_next_bookmark2:
  pMark = BMSetFindNextBookmark(pFile->pBMFuncFindCtx, &nCalcRow);

  /* We can not delete the current mark before advancing to the next */
  if (pMarkToRemove != NULL)
    BMListRemoveBookmark(pMarkToRemove);
  pMarkToRemove = NULL;

  if (pMark == NULL)
    goto _prepare_next_extraction;
  if (nCalcRow < nCurLine)
    goto _prepare_next_extraction;
  if (nCalcRow > nEndLine)
    goto _prepare_next_extraction;

  /* Filter out the not BMSetFuncNames bookmarks */
  if (pMark->pSet != pFile->pBMSetFuncNames)
    goto _next_bookmark2;

  /* Check if we didn't pass beyond the end of the region */
  if (nCalcRow > nEndLine)
    goto _exit;

  /* Check if the function from the bookmark is in the list freshly scanned */
  for (i = 0; i < nNumFunctions; ++i)
  {
    nRow = stFunctions[i].nLine;
    if (nCalcRow == nRow)
      goto _next_bookmark2;  /* This bookmark is still valid, move to next */
  }
  /* This bookmark no longer represents valid function name, mark for removing */
  pMarkToRemove = pMark;
  goto _next_bookmark2;

_prepare_next_extraction:
  if (nNumFunctions == 0)
  {
_exit:
    if (pMarkToRemove != NULL)
      BMListRemoveBookmark(pMarkToRemove);
    if (bUpdate)
      BookmarksInvalidate(pstFuncNames);
    return bNewMarksGlobal;
  }
  nCurLine = stFunctions[nNumFunctions - 1].nFuncEndLine;
  nCurPos = stFunctions[nNumFunctions - 1].nFuncEndPos;
  goto _scan;
}

/* ************************************************************************
   Function: ApplyFuncColors
   Description:
     To be used within TExtraColorInterf to highlight function names
     when displaying a page in the editor
*/
void ApplyFuncColors(const TFile *pFile, int bNewPage, int nLine,
  int nTopLine, int nWinHeight,
  TSynHInterf *pSynhInterf, struct ExtraColorInterf *pCtx)
{
  TBookmarksSet *pBMSetFuncNames;
  TBMFindNextCtx *pBMFindCtx;
  int nRow;
  TMarkLocation *pMark;
  TFunctionName *pFuncName;
  int nFirstLine;
  BOOLEAN bFirstTime;
  BOOLEAN bUserBookmark;

  pBMSetFuncNames = pCtx->pReserved1;
  pBMFindCtx = pCtx->pExtraCtx;
  bUserBookmark = FALSE;

  if (bNewPage)
  {
    nFirstLine = nLine - 1;
    if (nFirstLine < 0)
      nFirstLine = 0;
    /* BMSetFindFirstBookmark():
    1. establish the search context; 2. calc bFirstTime */
    pCtx->pReserved2 = BMSetFindFirstBookmark(NULL,
      pFile->sFileName, nFirstLine, 0, pBMFindCtx, &nRow);
    pMark = pCtx->pReserved2;
    /* bFirstTime: there are already some FuncName bookmarks for this page */
    bFirstTime = FALSE;
    if (pMark == NULL)
      bFirstTime = TRUE;
    RescanPageForFuncNames(pFile,
      nTopLine, nTopLine + nWinHeight, bFirstTime);
    /* BMSetFindFirstBookmark(): now establish the cache for
    the rest of the apply func */
    pCtx->pReserved2 = BMSetFindFirstBookmark(NULL,
      pFile->sFileName, nFirstLine, 0, pBMFindCtx, &nRow);
    pCtx->nReserved1 = nRow;
    if (pCtx->pReserved2 == NULL)
      pCtx->nReserved1 = -1;
    return;
  }

  if (pCtx->nReserved1 == -1)
    return;  /* No any bookmarks, BMSetFindFirstBookmark() returned NULL */

  /* Extract cached data */
  pMark = pCtx->pReserved2;
  nRow = pCtx->nReserved1;

  if (pMark == NULL || nLine != nRow)  /* We are not called for the same line? */
  {
    /* Obtain new data */
    BMSetRevertToFirst(pBMFindCtx);  /* not very expensive */
    BMSetNewSearchRow(pBMFindCtx, nLine);
_next_bookmark:
    pMark = BMSetFindNextBookmark(pBMFindCtx, &nRow);
    pCtx->pReserved2 = pMark;  /* Update the cache */
    pCtx->nReserved1 = nRow;
  }

  if (pMark == NULL)
    return;  /* No bookmarks */

  if (nRow != nLine)
    return;  /* No function name bookmark for this specific line */

  /*
  Here: process UserBookmarks or FunctionNames highlighting
  */
  if (pMark->pSet == pBMSetFuncNames)
  {
      pFuncName = pMark->pExtraRec;
      pSynhInterf->pfnPutAttr(COLOR_SFR,
        pFuncName->nNamePos,
        pFuncName->nNamePos + pFuncName->nNameLen - 1,
        pSynhInterf);
      if (bUserBookmark)  /* reapply bookmark indicator on top */
        pSynhInterf->pfnPutAttr(COLOR_BOOKMARK, 0, 0, pSynhInterf);
  }
  else
    if (pMark->pSet == &stUserBookmarks)
    {
      bUserBookmark = TRUE;
      pSynhInterf->pfnPutAttr(COLOR_BOOKMARK, 0, 0, pSynhInterf);
    }

  /* Process all bookmarks for this line of the file */
  goto _next_bookmark;
}

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2002, 2003, 2004
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

