/*

File: ini2.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 26th December, 1998
Descrition:
  Stores and loads all the options from the INI file.

*/

#include "global.h"
#include "disp.h"
#include "l2disp.h"
#include "history.h"
#include "bookm.h"
#include "ini.h"
#include "doctype.h"
#include "wrkspace.h"
#include "memory.h"
#include "l1opt.h"
#include "l1def.h"
#include "defs.h"
#include "options.h"
#include "main2.h"  /* for vermaj, vermin, ... */
#include "ini2.h"

static BOOLEAN bNewINIVersion;

BOOLEAN bDontStoreINIFile = FALSE;
BOOLEAN bDontStoreMasterINIFile = FALSE;

/* ************************************************************************
   Function: DisplayINIFileError
   Description:
     Displays a proper error message.
*/
static void DisplayINIFileError(TFile *pINIFile, int nErrorLine,
  int nErrorCode, BOOLEAN bSilent, dispc_t *disp)
{
  if (!bSilent)
  {
    ConsoleMessageProc(disp, NULL, MSG_ERROR | MSG_OK, pINIFile->sFileName,
      "(filename):%d: %s", nErrorLine + 1,
      sINIFileErrors[nErrorCode - 1]);
  }
}

typedef struct _version_desc
{
  int nCfgVersion;
} TVersionDesc;

/* ************************************************************************
   Function: LoadEditorVersion
   Description:
     Loads the [Version] section from the INI file.
     Only "CfgVersion" is loaded from the INI file.
*/
static BOOLEAN LoadEditorVersion(TFile *pINIFile,
  TVersionDesc *pVerDesc, BOOLEAN bSilent, dispc_t *disp)
{
  int nINILine;
  int nErrorCode;
  char Key[MAX_KEY_NAME_LEN];
  char Val[MAX_VAL_LEN];
  int nValResult;
  int nVal;
  BOOLEAN bResult;

  pVerDesc->nCfgVersion = 0;

  if ((nErrorCode = SearchSection(pINIFile, sSection_Version, &nINILine)) != 0)
  {
    if (nErrorCode != ERROR_NO_SUCH_SECTION)  /* If no such section EndOfSection() will exit */
      DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
    return FALSE;
  }

  bResult = FALSE;
  while (1)
  {
    ++nINILine;

    if (EndOfSection(pINIFile, nINILine))
      break;

    if ((nErrorCode = ParseINILine(GetLineText(pINIFile, nINILine), Key, Val)) != 0)
    {
      if (nErrorCode != ERROR_LINE_EMPTY)
        DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
      continue;
    }

    if (stricmp(Key, sKey_CfgVersion) == 0)
    {
      nValResult = ValStr(Val, &nVal, 10);
      if (!nValResult || Val <= 0)
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_NUMBER, bSilent, disp);
      else
      {
        pVerDesc->nCfgVersion = nVal;
        bResult = TRUE;
      }
      continue;
    }
  }
  return bResult;
}

/* ************************************************************************
   Function: StoreEditorVersion
   Description:
   Returns:
     0 -- [Version] section stored successfully or unchanged
     1 -- [Version] on disk is newer, not changed unless bForce is set
     2 -- Memory not enough for temp buffer
*/
static int
StoreEditorVersion(TFile *pINIFile,
                   BOOLEAN bForce,
                   TSectionBuf *pSec,
                   dispc_t *disp)
{
  TVersionDesc VerDesc;
  int nVerSectionPos;

  SectionReset(pSec);

  if (LoadEditorVersion(pINIFile, &VerDesc, TRUE, disp))
  {
    if (VerDesc.nCfgVersion > vercfg && !bForce)
      return 1;  /* on disk is newer */
    if (VerDesc.nCfgVersion == vercfg)
      return 0;  /* section unchanged */
  }

  if (!SectionPrintF(pSec, "[%s]\n", sSection_Version))
  {
_fail_printf:
    SectionReset(pSec);
    return 2;
  }
  if (!SectionPrintF(pSec, "%s = %d.%d.%d\n",
    sKey_Version, vermaj, vermin, verbld))
    goto _fail_printf;
  if (!SectionPrintF(pSec, "%s = %d\n\n", sKey_CfgVersion, vercfg))
    goto _fail_printf;

  nVerSectionPos = RemoveSection(pINIFile, sSection_Version);
  if (!SectionInsert(pINIFile, nVerSectionPos, pSec))
    goto _fail_printf;

  return 0;
}

/* ************************************************************************
   Function: LoadHistory
   Description:
     Loads a history contents from the INI file.
*/
static void LoadHistory(TFile *pINIFile, THistory *pHist,
  const char *sSectionName, int nMaxHistory, BOOLEAN bSilent, dispc_t *disp)
{
  int nINILine;
  char Key[MAX_KEY_NAME_LEN];
  char Val[MAX_VAL_LEN];
  char sBuf[_MAX_PATH];
  int nErrorCode;
  int nVal;
  BOOLEAN nValResult;
  BOOLEAN bTooLong;

  /*
  Init the history assuming there's nothing in the specific
  history section in the INI file.
  */
  InitHistory(pHist, nMaxHistory);

  if ((nErrorCode = SearchSection(pINIFile, sSectionName, &nINILine)) != 0)
  {
    if (nErrorCode != ERROR_NO_SUCH_SECTION)  /* If no such section EndOfSection() will exit */
    {
      DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
    }
    return;
  }

  while (1)
  {
    ++nINILine;

    if (EndOfSection(pINIFile, nINILine))
      break;

    if ((nErrorCode = ParseINILine(GetLineText(pINIFile, nINILine), Key, Val)) != 0)
    {
      if (nErrorCode != ERROR_LINE_EMPTY)
        DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
      continue;
    }

    /*
    MaxItems = x
    */
    if (stricmp(Key, sKey_MaxItems) == 0)
    {
      nValResult = ValStr(Val, &nVal, 10);
      if (!nValResult || Val <= 0)
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_NUMBER, bSilent, disp);
      else
        SetMaxHistoryItems(pHist, nVal);
      continue;
    }

    /*
    Line = text
    */
    if (stricmp(Key, sKey_Line) == 0)
    {
      ExtractQuotedString(Val, sBuf, "", sizeof(sBuf) - 1, &bTooLong);
      if (bTooLong)
        DisplayINIFileError(pINIFile, nINILine, ERROR_STRING_TOO_LONG, bSilent, disp);
      else
        AddHistoryLine(pHist, sBuf);
      continue;
    }

    DisplayINIFileError(pINIFile, nINILine, ERROR_UNRECOGNIZED_KEY, bSilent, disp);
  }
}

typedef struct _StoreHistoryItemCtx
{
  int nError;
  TSectionBuf *pSec;
} TStoreHistoryItemCtx;

/* ************************************************************************
   Function: StoreHistItem
   Description:
*/
static void StoreHistItem(const char *sHistLine, void *_pCtx)
{
  char sBuf[_MAX_PATH];
  TStoreHistoryItemCtx *pCtx;

  pCtx = _pCtx;
  PrintQuotedString(sHistLine, sBuf);
  pCtx->nError = 0;
  if (!SectionPrintF(pCtx->pSec, "%s = %s\n", sKey_Line, sBuf))
    pCtx->nError = 2;
}

/* ************************************************************************
   Function: StoreHistory
   Description:
     Stores a specific history section in the INI file.
  Returns:
    0 -- A history section stored successfully or unchanged
    2 -- Memory not enough for temp buffer
*/
static int StoreHistory(TFile *pINIFile, TSectionBuf *pSec,
  THistory *pHist, const char *sHistoryName, int nMaxItems)
{
  TStoreHistoryItemCtx StoreHistCtx;
  int nSectionPos;

  SectionReset(pSec);
  memset(&StoreHistCtx, 0, sizeof(StoreHistCtx));
  StoreHistCtx.pSec = pSec;

  if (!SectionPrintF(pSec, "[%s]\n", sHistoryName))
  {
_failprintf:
    SectionReset(pSec);
    return 2;
  }
  if (pHist->nMaxItems != nMaxItems)
  {
    if (!SectionPrintF(pSec, "%s = %d\n", sKey_MaxItems, pHist->nMaxItems))
      goto _failprintf;
  }

  HistoryForEach(pHist, StoreHistItem, FALSE, &StoreHistCtx);

  if (StoreHistCtx.nError == 0)
  {
    nSectionPos = RemoveSection(pINIFile, sHistoryName);
    if (!SectionInsert(pINIFile, nSectionPos, pSec))
      goto _failprintf;
  }

  return StoreHistCtx.nError;
}

/* ************************************************************************
   Function: LoadMRUFiles
   Description:
     Load all MRUFiles entries from the INI file.
*/
static void LoadMRUFiles(TFile *pINIFile, BOOLEAN bSilent, dispc_t *disp)
{
  int nINILine;
  char Key[MAX_KEY_NAME_LEN];
  char sBuf[_MAX_PATH];
  char Val[MAX_VAL_LEN];
  char *sFileName;
  char *sCol;
  char *sRow;
  char *sTopLine;
  char *sClosed;
  char *sCurrent;
  char *sForceReadOnly;
  char *sCopy;
  char *p;
  int nCol;
  int nRow;
  int nTopLine;
  int nCopy;
  BOOLEAN bClosed;
  BOOLEAN bCurrent;
  BOOLEAN bForceReadOnly;
  BOOLEAN bTooLong;
  int nErrorCode;
  int nVal;
  BOOLEAN bValResult;

  bClosed = TRUE;
  bCurrent = FALSE;

  if ((nErrorCode = SearchSection(pINIFile, sSection_MRUFiles, &nINILine)) != 0)
  {
    if (nErrorCode != ERROR_NO_SUCH_SECTION)
    {
_display_error:
      DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
    }
    return;
  }

  /*
  After cfg version 2, the MRU entries have a new format
  */
  if (nINIVersion < 2)
  {
    bNewINIVersion = TRUE;
    return;
  }

  while (1)
  {
    ++nINILine;
    if (EndOfSection(pINIFile, nINILine))
      break;
    if ((nErrorCode = ParseINILine(GetLineText(pINIFile, nINILine), Key, Val)) != 0)
    {
      if (nErrorCode != ERROR_LINE_EMPTY)
        goto _display_error;
      continue;
    }
    if (stricmp(Key, sKEY_MRUMax) == 0)
    {
      bValResult = ValStr(Val, &nVal, 10);
      if (!bValResult || nVal <= 0)
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_NUMBER, bSilent, disp);
      else
      {
        if (nVal >= MAX_MRU_FILES)
          SetMaxMRUItems(pMRUFilesList, nVal);
      }
      continue;
    }
    if (stricmp(Key, sKEY_MRUFile) == 0)
    {
      /* Parse the INI file line */
      p = ExtractQuotedString(Val, sBuf, ", ", sizeof(sBuf) - 1, &bTooLong);
      if (bTooLong)
      {
        DisplayINIFileError(pINIFile, nINILine, ERROR_STRING_TOO_LONG, bSilent, disp);
        continue;
      }
      sFileName = sBuf;
      *p = '\0';
      sCol = strtok(p + 1, ", ");
      sRow = strtok(NULL, ", ");
      sTopLine = strtok(NULL, ", ");
      sClosed = strtok(NULL, ", ");
      sCurrent = strtok(NULL, ", ");
      sForceReadOnly = strtok(NULL, ", ");

      /* Check for copy number -- for example file.c:2 */
      nCopy = 0;
      if ((sCopy = strrchr(sFileName, ':')) != NULL)
      {
        /* Check for drive ':' delimiter -- 'c:' */
        /* Side effect -- file 'c:1' could not reside in MRU list */
        if (sCopy - sFileName != 1)
        {
          if (!ValStr(sCopy + 1, &nCopy, 10))
            goto _val_error;
          if (nCopy < 2)
            goto _val_error;
          --nCopy;
          *sCopy = '\0';  /* Strip the file name */
        }
      }

      if (sClosed == NULL || sCurrent == NULL || sForceReadOnly == NULL ||
          sCol == NULL || sRow == NULL || sTopLine == NULL)
      {
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_MRUENTRY, bSilent, disp);
        continue;
      }

      bValResult = ValStr(sCol, &nCol, 10);
      bValResult = bValResult && ValStr(sRow, &nRow, 10);
      bValResult = bValResult && ValStr(sTopLine, &nTopLine, 10);
      if (!bValResult)
      {
_val_error:
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_NUMBER, bSilent, disp);
        continue;
      }

      if (stricmp(sClosed, sVal_Closed) == 0)
        bClosed = TRUE;
      else
        if (stricmp(sClosed, sVal_Opened) == 0)
          bClosed = FALSE;
        else
        {
          DisplayINIFileError(pINIFile, nINILine, ERROR_OPCLOSED_EXPECTED, bSilent, disp);
          continue;
        }

      if (stricmp(sCurrent, sVal_Current) == 0)
        bCurrent = TRUE;
      else
        if (stricmp(sCurrent, "0") == 0)
          bCurrent = FALSE;
        else
        {
          DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_CURRENT, bSilent, disp);
          continue;
        }

      if (stricmp(sForceReadOnly, sVal_ForceReadOnly) == 0)
        bForceReadOnly = TRUE;
      else
        if (stricmp(sForceReadOnly, "0") == 0)
          bForceReadOnly = FALSE;
        else
        {
          DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_FREADONLY, bSilent, disp);
          continue;
        }

      AddFileToMRUList(pMRUFilesList, sFileName, nCopy, bClosed, bCurrent,
        bForceReadOnly, nCol, nRow, nTopLine, FALSE);
      continue;
    }
    DisplayINIFileError(pINIFile, nINILine, ERROR_UNRECOGNIZED_KEY, bSilent, disp);
  }
}

typedef struct _StoreMRUItemCtx
{
  BOOLEAN bStoreFullPath;
  int nError;
  TSectionBuf *pSec;
} TStoreMRUItemCtx;

/* ************************************************************************
   Function: StoreMRUItem
   Description:
     A call-back function to store all the items of a mru list.
*/
static void StoreMRUItem(TMRUListItem *pItem, void *_pCtx)
{
  char sCopy[20];
  char buf[_MAX_PATH];
  char sFileName[_MAX_PATH];
  TStoreMRUItemCtx *pCtx;

  pCtx = _pCtx;
  if (pCtx->nError != 0)  /* Some of the previous calls failed? */
    return;

  sCopy[0] = '\0';
  if (pItem->nCopy > 0)
    sprintf(sCopy, ":%d", pItem->nCopy + 1);

  strcpy(buf, pItem->sFileName);
  if (pCtx->bStoreFullPath)
    GetFullPath(buf);
  if (strchr(buf, ' ') != NULL)
    sprintf(sFileName, "\"%s\"", buf);
  else
    strcpy(sFileName, buf);

  pCtx->nError = 0;
  if (!SectionPrintF(pCtx->pSec,
    "%s = %s%s, %d, %d, %d, %s, %s, %s\n", sKEY_MRUFile,
    sFileName, sCopy,
    pItem->nCol, pItem->nRow, pItem->nTopLine,
    pItem->bClosed ? sVal_Closed : sVal_Opened,
    pItem->bCurrent ? sVal_Current : "0",
    pItem->bForceReadOnly ? sVal_ForceReadOnly : "0"))
    pCtx->nError = 2;
}

/* ************************************************************************
   Function: StoreMRUFiles
   Description:
     Stores all the MRUFiles entryes in	the INI file.
  Returns:
    0 -- MRUFiles section stored successfully or unchanged
    2 -- Memory not enough for temp buffer
*/
static int StoreMRUFiles(TFile *pINIFile, TSectionBuf *pSec)
{
  char sCurDir[_MAX_PATH];
  TStoreMRUItemCtx StoreItemCtx;
  int nSectionPos;

  SectionReset(pSec);
  memset(&StoreItemCtx, 0, sizeof(StoreItemCtx));
  StoreItemCtx.pSec = pSec;

  if (!SectionPrintF(pSec, "[%s]\n", sSection_MRUFiles))
  {
_failprintf:
    SectionReset(pSec);
    return 2;
  }

  if (pMRUFilesList->nMaxNumberOfItems != MAX_MRU_FILES)
  {
    if (!SectionPrintF(pSec, "%s = %d\n", sKEY_MRUMax, pMRUFilesList->nMaxNumberOfItems))
      goto _failprintf;
  }

  GetCurDir(sCurDir);
  AddTrailingSlash(sCurDir);
  StoreItemCtx.bStoreFullPath = FALSE;
  if (filestrcmp(sINIFilePath, sCurDir) != 0)
    StoreItemCtx.bStoreFullPath = TRUE;

  MRUListForEach(pMRUFilesList, StoreMRUItem, FALSE, &StoreItemCtx);

  if (StoreItemCtx.nError == 0)
  {
    nSectionPos = RemoveSection(pINIFile, sSection_MRUFiles);
    if (!SectionInsert(pINIFile, nSectionPos, pSec))
      goto _failprintf;
  }

  return StoreItemCtx.nError;
}

/* ************************************************************************
   Function: DetectOnOff
   Description:
*/
static BOOLEAN DetectOnOff(const char *pOnOff, BOOLEAN *bFlag)
{
  ASSERT(bFlag != NULL);

  if (pOnOff == NULL)
    return FALSE;

  if (stricmp(pOnOff, sOn) == 0)
  {
    *bFlag = TRUE;
    return TRUE;
  }
  if (stricmp(pOnOff, sOff) == 0)
  {
    *bFlag = FALSE;
    return TRUE;
  }
  return FALSE;
}

/* ************************************************************************
   Function: DetectYesNo
   Description:
*/
static BOOLEAN DetectYesNo(const char *pYesNo, int *bFlag)
{
  ASSERT(bFlag != NULL);

  if (pYesNo == NULL)
    return FALSE;

  if (stricmp(pYesNo, sYes) == 0)
  {
    *bFlag = TRUE;
    return TRUE;
  }
  if (stricmp(pYesNo, sNo) == 0)
  {
    *bFlag = FALSE;
    return TRUE;
  }
  return FALSE;
}

/* ************************************************************************
   Function: LoadEditorOptions
   Description:
*/
static void LoadEditorOptions(TFile *pINIFile, BOOLEAN bSilent, dispc_t *disp)
{
  int nINILine;
  int nErrorCode;
  char Key[MAX_KEY_NAME_LEN];
  char Val[MAX_VAL_LEN];
  int nVal;
  BOOLEAN bValResult;

  ASSERT(pINIFile != NULL);

  /*
  Work options variables are initialized with default, copy to XXX_Opt.
  This is in case there are no values for some parameters in w.ini
  */
  CopyFromEditorOptions();

  if ((nErrorCode = SearchSection(pINIFile, sSection_EditorOptions, &nINILine)) != 0)
  {
    if (nErrorCode != ERROR_NO_SUCH_SECTION)
    {
_display_error:
      DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
    }
    else
    {
      /*
      No [EditorOptions] section -- default editor options have
      already took place
      */
    }
    return;
  }

  while (1)
  {
    ++nINILine;
    if (EndOfSection(pINIFile, nINILine))
      break;

    if ((nErrorCode = ParseINILine(GetLineText(pINIFile, nINILine), Key, Val)) != 0)
    {
      if (nErrorCode != ERROR_LINE_EMPTY)
        goto _display_error;
      continue;
    }

    if (stricmp(Key, sKey_Backup) == 0)
    {
      if (!DetectOnOff(Val, &bBackup_Opt))
      {
_display_on_off_error:
        DisplayINIFileError(pINIFile, nINILine, ERROR_ON_OFF_EXPECT, bSilent, disp);
      }
      continue;
    }

    #define LoadOption(sKey, bOpt)\
    if (stricmp(Key, sKey) == 0)\
    {\
      if (!DetectOnOff(Val, &bOpt))\
        goto _display_on_off_error;\
      continue;\
    }

    LoadOption(sKey_RestoreLastFiles, bRestoreLastFiles_Opt);
    LoadOption(sKey_PersistentBlocks, bPersistentBlocks_Opt);
    LoadOption(sKey_OverwriteBlocks, bOverwriteBlocks_Opt);
    LoadOption(sKey_SyntaxHighlighting, bSyntaxHighlighting_Opt);
    LoadOption(sKey_MatchPairHighlighting, bMatchPairHighlighting_Opt);
    LoadOption(sKey_IF0Highlighting, bIF0Highlighting_Opt);
    LoadOption(sKey_AscendingSort, bAscendingSort_Opt);
    LoadOption(sKey_CaseSensitiveSort, bCaseSensitiveSort_Opt);

    LoadOption(sKey_CombineUndo, bCombineUndo_Opt);

    LoadOption(sKey_StrictCheck, bStrictCheck_Opt);
    LoadOption(sKey_ConsequtiveWinFiles, bConsequtiveWinFiles_Opt);

    LoadOption(sKey_RemoveTrailingBlanks, bRemoveTrailingBlanks_Opt);
    LoadOption(sKey_StartWithEmptyFile, bStartWithEmptyFile_Opt);

    #undef LoadOption

    if (stricmp(Key, sKey_RightMargin) == 0)
    {
      bValResult = ValStr(Val, &nVal, 10);
      if (!bValResult)
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_NUMBER, bSilent, disp);
      else
        if (nVal < 0 || nVal >= MAX_RIGHT_MARGIN_VAL)
          DisplayINIFileError(pINIFile, nINILine, ERROR_INV_RMARG, bSilent, disp);
        else
          nRightMargin_Opt = nVal;
      continue;
    }

    if (stricmp(Key, sKey_RecoveryTime) == 0)
    {
      bValResult = ValStr(Val, &nVal, 10);
      if (!bValResult)
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_NUMBER, bSilent, disp);
      else
        if (nVal < 0 || nVal >= MAX_RECOVERY_TIME_VAL)
          DisplayINIFileError(pINIFile, nINILine, ERROR_INV_RTIME, bSilent, disp);
        else
          nRecoveryTime_Opt = nVal;
      continue;
    }

    if (stricmp(Key, sKey_FileSaveMode) == 0)
    {
      if (stricmp(Val, sVal_Auto) == 0)
      {
        nFileSaveMode_Opt = -1;
        continue;
      }

      if (stricmp(Val, sVal_LF) == 0 || stricmp(Val, sVal_Unix) == 0)
      {
        nFileSaveMode_Opt = LFtype;
        continue;
      }

      if (stricmp(Val, sVal_CR) == 0 || stricmp(Val, sVal_Mac) == 0)
      {
        nFileSaveMode_Opt = CRtype;
        continue;
      }

      if (stricmp(Val, sVal_CRLF) == 0 || stricmp(Val, sVal_DOS) == 0)
      {
        nFileSaveMode_Opt = CRLFtype;
      }

      DisplayINIFileError(pINIFile, nINILine, ERROR_INV_SAVEMODE, bSilent, disp);
      continue;
    }

    DisplayINIFileError(pINIFile, nINILine, ERROR_UNRECOGNIZED_KEY, bSilent, disp);
  }

  CopyToEditorOptions();
}

/* ************************************************************************
   Function: GetOnOff
   Description:
*/
static const char *GetOnOff(BOOLEAN bOn)
{
  return (bOn ? sOn : sOff);
}

#ifdef _NON_TEXT
/* ************************************************************************
   Function: GetYesNo
   Description:
*/
static const char *GetYesNo(int bYes)
{
  return (bYes ? sYes : sNo);
}
#endif

/* ************************************************************************
   Function: StoreEditorOptions
   Description:
   Returns:
     0 -- section is fine
     2 -- no-memory
*/
static int StoreEditorOptions(TFile *pINIFile, TSectionBuf *pSec)
{
  int nSectionPos;

  SectionReset(pSec);

  if (!SectionPrintF(pSec, "[%s]\n", sSection_EditorOptions))
  {
_failprintf:
    SectionReset(pSec);
    return 2;
  }

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_Backup, GetOnOff(bBackup_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_RestoreLastFiles, GetOnOff(bRestoreLastFiles_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %d\n", sKey_RecoveryTime, nRecoveryTime_Opt))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_PersistentBlocks, GetOnOff(bPersistentBlocks_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_OverwriteBlocks, GetOnOff(bOverwriteBlocks_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_SyntaxHighlighting, GetOnOff(bSyntaxHighlighting_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_MatchPairHighlighting, GetOnOff(bMatchPairHighlighting_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_IF0Highlighting, GetOnOff(bIF0Highlighting_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_AscendingSort, GetOnOff(bAscendingSort_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_CaseSensitiveSort, GetOnOff(bCaseSensitiveSort_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %d\n",	sKey_RightMargin, nRightMargin_Opt))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_CombineUndo, GetOnOff(bCombineUndo_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_StrictCheck, GetOnOff(bStrictCheck_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_RemoveTrailingBlanks, GetOnOff(bRemoveTrailingBlanks_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_ConsequtiveWinFiles, GetOnOff(bConsequtiveWinFiles_Opt)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_FileSaveMode, sEOLTypes[nFileSaveMode_Opt + 1]))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_StartWithEmptyFile, GetOnOff(bStartWithEmptyFile_Opt)))
    goto _failprintf;

  nSectionPos = RemoveSection(pINIFile, sSection_EditorOptions);
  if (!SectionInsert(pINIFile, nSectionPos, pSec))
    goto _failprintf;

  return 0;
}

typedef struct _StoreBookmarkCtx
{
  BOOLEAN bStoreFullPath;
  int nError;
  TSectionBuf *pSec;
} TStoreBookmarkCtx;

/* ************************************************************************
   Function: StoreBookmark
   Description:
     A call-back function for BMListForEach
*/
static BOOLEAN StoreBookmark(TMarkLocation *pMark, int nRow, void *_pCtx)
{
  char sPathName[_MAX_PATH];
  TStoreBookmarkCtx *pCtx;

  pCtx = _pCtx;
  if (pCtx->nError != 0)  /* Some of the previous calls failed? */
    return FALSE;

  if (pCtx->bStoreFullPath)
    strcpy(sPathName, pMark->pFileName->sFileName);
  else
    ShrinkPath(pMark->pFileName->sFileName, sPathName, 0, TRUE);
  pCtx->nError = 0;
  if (!SectionPrintF(pCtx->pSec,
    "%s = %s, %d,%s\n", sKEY_Bookmark,
    sPathName, nRow, pMark->psContent))
    pCtx->nError = 2;
  return TRUE;
}

/* ************************************************************************
   Function: StoreBookmarks
   Description:
*/
static int StoreBookmarks(TFile *pINIFile, TSectionBuf *pSec,
  TBookmarksSet *pstBMSet, const char *psSectionName)
{
  char sCurDir[_MAX_PATH];
  TStoreBookmarkCtx StoreBookmarkCtx;
  int nSectionPos;

  SectionReset(pSec);
  memset(&StoreBookmarkCtx, 0, sizeof(StoreBookmarkCtx));
  StoreBookmarkCtx.pSec = pSec;

  GetCurDir(sCurDir);
  AddTrailingSlash(sCurDir);
  StoreBookmarkCtx.bStoreFullPath = FALSE;
  if (filestrcmp(sINIFilePath, sCurDir) != 0)
    StoreBookmarkCtx.bStoreFullPath = TRUE;

  if (!SectionPrintF(pSec, "[%s]\n", psSectionName))
  {
_failprintf:
    SectionReset(pSec);
    return 2;
  }

  BMListForEach(StoreBookmark, pstBMSet, FALSE, &StoreBookmarkCtx);

  if (StoreBookmarkCtx.nError == 0)
  {
    nSectionPos = RemoveSection(pINIFile, psSectionName);
    if (!SectionInsert(pINIFile, nSectionPos, pSec))
      goto _failprintf;
  }

  return StoreBookmarkCtx.nError;
}

/* ************************************************************************
   Function: LoadBookmarks
   Description:
*/
static void LoadBookmarks(TFile *pINIFile,
  TBookmarksSet *pstBMSet, const char *psSection, BOOLEAN bSilent, dispc_t *disp)
{
  int nINILine;
  int nErrorCode;
  char Key[MAX_KEY_NAME_LEN];
  char Val[MAX_VAL_LEN];
  char psFileName[_MAX_PATH];
  char *_psFileName;
  char *psLine;
  char *psContent;
  char *psContentInHeap;
  int nLine;

  ASSERT(pINIFile != NULL);

  if ((nErrorCode = SearchSection(pINIFile, psSection, &nINILine)) != 0)
  {
    if (nErrorCode != ERROR_NO_SUCH_SECTION)  /* If no such section EndOfSection() will exit */
    {
      DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
    }
    return;
  }

  while (1)
  {
    ++nINILine;
    if (EndOfSection(pINIFile, nINILine))
      break;

    if ((nErrorCode = ParseINILine(GetLineText(pINIFile, nINILine), Key, Val)) != 0)
    {
      if (nErrorCode != ERROR_LINE_EMPTY)
        DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
      continue;
    }

    if (stricmp(Key, sKEY_Bookmark) == 0)
    {
      _psFileName = strtok(Val, ", ");
      psLine = strtok(NULL, ", ");
      psContent = strtok(NULL, ",");

      if (!ValStr(psLine, &nLine, 10))
      {
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_NUMBER, bSilent, disp);
        continue;
      }
      if (strlen(_psFileName) > _MAX_PATH)
      {
        DisplayINIFileError(pINIFile, nINILine, ERROR_FILENAME_TOOLONG_BM, bSilent, disp);
        continue;
      }
      strcpy(psFileName, _psFileName);
      GetFullPath(psFileName);
      psContentInHeap = NULL;
      BMListInsert(psFileName, nLine, 0, -1, psContent, NULL, 0,
        pstBMSet, NULL, 0);
      continue;
    }

    DisplayINIFileError(pINIFile, nINILine, ERROR_UNRECOGNIZED_KEY, bSilent, disp);
  }
}

/* ************************************************************************
   Function: StoreElapsedTime
   Description:
*/
static int StoreElapsedTime(TFile *pINIFile, TSectionBuf *pSec, dispc_t *disp)
{
  int nSectionPos;
  disp_elapsed_time_t t;

  SectionReset(pSec);

  if (!SectionPrintF(pSec, "[%s]\n", sSection_ElapsedTime))
  {
_failprintf:
    SectionReset(pSec);
    return 2;
  }

  disp_elapsed_time_get(disp, &t);
  if (!SectionPrintF(pSec,
    "%s = %d:%02d:%02d\n", sKey_Time, t.hours, t.minutes, t.seconds))
    goto _failprintf;

  nSectionPos = RemoveSection(pINIFile, sSection_ElapsedTime);
  if (!SectionInsert(pINIFile, nSectionPos, pSec))
    goto _failprintf;

  return 0;
}

/* ************************************************************************
   Function: LoadElapsedTime
   Description:
*/
static void LoadElapsedTime(TFile *pINIFile, BOOLEAN bSilent, dispc_t *disp)
{
  int nINILine;
  int nErrorCode;
  char Key[MAX_KEY_NAME_LEN];
  char Val[MAX_VAL_LEN];
  char *pHour;
  char *pMin;
  char *pSec;
  disp_elapsed_time_t t;
  int nHour;
  int nMin;
  int nSec;

  ASSERT(pINIFile != NULL);

  if ((nErrorCode = SearchSection(pINIFile, sSection_ElapsedTime, &nINILine)) != 0)
  {
    if (nErrorCode != ERROR_NO_SUCH_SECTION)  /* If no such section EndOfSection() will exit */
    {
      DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
    }
    return;
  }

  while (1)
  {
    ++nINILine;
    if (EndOfSection(pINIFile, nINILine))
      break;

    if ((nErrorCode = ParseINILine(GetLineText(pINIFile, nINILine), Key, Val)) != 0)
    {
      if (nErrorCode != ERROR_LINE_EMPTY)
        DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
      continue;
    }

    if (stricmp(Key, sKey_Time) == 0)
    {
      pHour = strtok(Val, " :");
      pMin = strtok(NULL, ":");
      pSec = strtok(NULL, " :");

      if (!ValStr(pHour, &nHour, 10) || !ValStr(pMin, &nMin, 10)
	|| !ValStr(pSec, &nSec, 10))
      {
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_NUMBER, bSilent, disp);
      }
      else
      {
        t.hours = nHour;
	t.minutes = nMin;
	t.seconds = nSec;
        disp_elapsed_time_set(disp, &t);
      }
      continue;
    }

    DisplayINIFileError(pINIFile, nINILine, ERROR_UNRECOGNIZED_KEY, bSilent, disp);
  }
}

/* ************************************************************************
   Function: LoadWorkspace
   Description:
     Loads the workspace options. If the file pINIFile is empty all
     the parameters get their default values.

   INI file changes:
   Version 2 -- introduced new incompatible DocTypes storage format;
*/
void LoadWorkspace(TFile *pINIFile, dispc_t *disp)
{
  TVersionDesc VerDesc;

  ASSERT(pINIFile != NULL);

  bNewINIVersion = FALSE;
  LoadEditorVersion(pINIFile, &VerDesc, FALSE, disp);
  nINIVersion = VerDesc.nCfgVersion;

  if (nINIVersion > vercfg)
  {
    /*
    We can not process newer configuraition formats.
    Get the lates executable.
    */
    ConsoleMessageProc(disp, NULL, MSG_ERROR | MSG_INFO, NULL, sINIFormatIsNew);
    bDontStoreINIFile = TRUE;
    return;
  }

  LoadEditorOptions(pINIFile, FALSE, disp);
  LoadMRUFiles(pINIFile, FALSE, disp);
  LoadHistory(pINIFile, pFileOpenHistory,
    sSection_FileOpenHistory, MAX_HISTORY_ITEMS, FALSE, disp);
  LoadHistory(pINIFile, pFindHistory,
    sSection_FindHistory, MAX_HISTORY_ITEMS, FALSE, disp);
  LoadHistory(pINIFile, pCalculatorHistory,
    sSection_CalculatorHistory, MAX_CALCULATOR_HISTORY_ITEMS, FALSE, disp);
  LoadHistory(pINIFile, pFindInFilesHistory,
    sSection_FindInFilesHistory, MAX_FINDINFILES_HISTORY_ITEMS, FALSE, disp);
  LoadBookmarks(pINIFile, &stUserBookmarks, sSection_UserBookmarks, FALSE, disp);
  LoadElapsedTime(pINIFile, FALSE, disp);

  if (bNewINIVersion)
    ConsoleMessageProc(disp, NULL, MSG_ERROR | MSG_INFO, NULL, sINIFormatChanged);

  if (pINIFile->bReadOnly)
    bDontStoreINIFile = TRUE;
}

/* ************************************************************************
   Function: StoreWorkspace
   Description:
     Stores workspace parameters and options to the INI file.
*/
void StoreWorkspace(dispc_t *disp)
{
  TFile *pINIFile;
  TSectionBuf Sec;

  ConsoleMessageProc(disp, NULL, MSG_STATONLY | MSG_INFO, sINIFileName, sSaveOptions);

  if (bDontStoreINIFile)
  {
    ConsoleMessageProc(disp, NULL, MSG_ERROR | MSG_INFO, NULL, sINIFileNotStored);
    return;
  }

  pINIFile = CreateINI(sINIFileName);  /* Allocate an empty file structure */
  if (!OpenINI(pINIFile, disp))  /* Load the INI file contents */
    ;  /* Well, nothing, we just keep going with the empty file */

  if (!SectionBegin(&Sec))
    goto _exit;  /* TODO: show no-memory at exit-code 2 */

  if (StoreEditorVersion(pINIFile, FALSE, &Sec, disp) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  if (StoreEditorOptions(pINIFile, &Sec) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  if (StoreMRUFiles(pINIFile, &Sec) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  if (StoreHistory(pINIFile, &Sec, pFileOpenHistory, sSection_FileOpenHistory, MAX_HISTORY_ITEMS) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  if (StoreHistory(pINIFile, &Sec, pFindHistory, sSection_FindHistory, MAX_HISTORY_ITEMS) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  if (StoreHistory(pINIFile, &Sec, pCalculatorHistory, sSection_CalculatorHistory, MAX_CALCULATOR_HISTORY_ITEMS) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  if (StoreHistory(pINIFile, &Sec, pFindInFilesHistory, sSection_FindInFilesHistory, MAX_FINDINFILES_HISTORY_ITEMS) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  if (StoreBookmarks(pINIFile, &Sec, &stUserBookmarks, sSection_UserBookmarks) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  if (StoreElapsedTime(pINIFile, &Sec, disp) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */

_exit2:
  SectionDisposeBuf(&Sec);

_exit:
  StoreINI(pINIFile, disp);
  CloseINI(pINIFile);
}

/* ************************************************************************
   Function: EnterDocItem
   Description:
*/
static void
EnterDocItem(TFile *pINIFile,
             int nINILine,
             TDocType *pDocItem,
             BOOLEAN bSilent,
             dispc_t *disp)
{
  char Key[MAX_KEY_NAME_LEN];
  char Val[MAX_VAL_LEN];
  int nErrorCode;
  char *pExt;
  char *pDocType;
  char *pTabSize;
  char *pUseTabs;
  char *pOptimalFill;
  char *pAutoIndent;
  char *pBackspaceUnindent;
  char *pCursorThroughTabs;
  char *pWordWrap;
  char *p;
  int nType;
  BOOLEAN bResult;

  ASSERT(pINIFile != NULL);
  ASSERT(nINILine > 0);
  ASSERT(pDocItem != NULL);

  pDocItem->nType = -1;  /* Indicate nothing loaded */

  if ((nErrorCode = ParseINILine(GetLineText(pINIFile, nINILine), Key, Val)) != 0)
  {
    if (nErrorCode != ERROR_LINE_EMPTY)
      DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
    return;
  }

  if (stricmp(Key, sKey_Doc) !=	0)
  {
    DisplayINIFileError(pINIFile, nINILine, ERROR_UNRECOGNIZED_KEY, bSilent, disp);
    return;
  }

  pExt = strtok(Val, ", ");
  pDocType = strtok(NULL, ", ");
  pTabSize = strtok(NULL, ", ");
  pUseTabs = strtok(NULL, ", ");
  pOptimalFill = strtok(NULL, ", ");
  pAutoIndent = strtok(NULL, ", ");
  pBackspaceUnindent = strtok(NULL, ", ");
  pCursorThroughTabs = strtok(NULL, ", ");
  pWordWrap = strtok(NULL, ", ");

  if (pExt == NULL || pDocType == NULL || pTabSize == NULL || pUseTabs == NULL ||
    pOptimalFill == NULL || pAutoIndent == NULL || pBackspaceUnindent == NULL ||
    pCursorThroughTabs == NULL || pWordWrap == NULL)
  {
    DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_DOCENTRY, bSilent, disp);
    return;
  }

  if (pExt[0] != '\x27')  /* ' */
  {
error1:
    DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_EXT, bSilent, disp);
    return;
  }

  /* Strip the enclosing ' */
  ++pExt;  /* Skip the first ' */
  p = strchr(pExt, '\x27');  /* Search for the last ' */
  if (!p)
    goto error1;
  *p = '\0';  /* Remove the last ' */

  /* Export to pDocItem->sExt */

  if (strlen(pExt) > MAX_EXT_LEN - 1)
  {
    DisplayINIFileError(pINIFile, nINILine, ERROR_EXT_TOO_LONG, bSilent, disp);
    return;
  }
  strcpy(pDocItem->sExt, pExt);

  if (pDocType[0] != '\x27')  /* ' */
  {
error2:
    DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_FTYPE, bSilent, disp);
    return;
  }

  /* Strip the enclosing ' */
  ++pDocType;  /* Skip the first ' */
  p = strchr(pDocType, '\x27');  /* Search for the last ' */
  if (!p)
    goto error2;
  *p = '\0';  /* Remove the last ' */

  /* Detect the file type */
  nType = FindSyntaxType(pDocType);
  if (nType < 0)  /* Document type was not recognized */
  {
    if (strcmp(pDocType, "none") == 0)  /* Legacy configuration contains "none" */
      nType = tyPlain;
    else
      //DisplayINIFileError(pINIFile, nINILine, ERROR_UNRECOGNIZED_FTYPE, bSilent);
      nType = tyPlain;
  }

  if (!ValStr(pTabSize, &pDocItem->nTabSize, 10))
  {
    DisplayINIFileError(pINIFile, nINILine, ERROR_BAD_TAB, bSilent, disp);
    return;
  }

  bResult = DetectOnOff(pUseTabs, &pDocItem->bUseTabs);
  bResult = bResult && DetectOnOff(pOptimalFill, &pDocItem->bOptimalFill);
  bResult = bResult && DetectOnOff(pAutoIndent, &pDocItem->bAutoIndent);
  bResult = bResult && DetectOnOff(pBackspaceUnindent, &pDocItem->bBackspaceUnindent);
  bResult = bResult && DetectOnOff(pCursorThroughTabs, &pDocItem->bCursorThroughTabs);
  bResult = bResult && DetectOnOff(pWordWrap, &pDocItem->bWordWrap);
  if (!bResult)
  {
    DisplayINIFileError(pINIFile, nINILine, ERROR_ON_OFF_EXPECT, bSilent, disp);
    return;
  }

  pDocItem->nType = nType;
}

/* ************************************************************************
   Function: LoadDocTypes
   Description:
     Processes [DocumentTypeSet] section.
     Processes [DefaultDocument] section.

   Returns: FALSE - no options were loaded, defaults used.
*/
static BOOLEAN
LoadDocTypes(TFile *pINIFile, BOOLEAN bSilent, dispc_t *disp)
{
  int nINILine;
  int nErrorCode;
  TDocType *pDocDest;
  TDocType DocItem;
  BOOLEAN bResult;

  ASSERT(pINIFile != NULL);

  bResult = TRUE;
  if ((nErrorCode = SearchSection(pINIFile, sSection_DocTypeSet, &nINILine)) != 0)
  {
    if (nErrorCode != ERROR_NO_SUCH_SECTION)
      DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
    /* No [DocumentTypeSet] section -- set default document set */
_set_default_documents:
    CopyDocumentTypeSet(DocumentTypes_Opt, DefaultDocuments);
    bResult = FALSE;
    goto section2;
  }

  pDocDest = DocumentTypes_Opt;
  while (1)
  {
    ++nINILine;
    if (EndOfSection(pINIFile, nINILine))
      break;
    EnterDocItem(pINIFile, nINILine, &DocItem, bSilent, disp);
    if (DocItem.nType != -1)
    {
      memcpy(pDocDest, &DocItem, sizeof(TDocType));
      pDocDest++;
    }
  }
  SET_END_OF_DOC_LIST(pDocDest);  /* Mark the end of the doc type set */
  if (pDocDest - DocumentTypes_Opt == 0)  /* There were no entries in this section */
  {
    bResult = FALSE;
    goto _set_default_documents;
  }

section2:
  if ((nErrorCode = SearchSection(pINIFile, sSection_DefaultDoc, &nINILine)) != 0)
  {
    if (nErrorCode != ERROR_NO_SUCH_SECTION)
      DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
    /* No [DefaultDocument] section -- set default document setings */
_set_default_doc_options:
    memcpy(&DefaultDocOptions_Opt, &_DefaultDocOptions, sizeof(TDocType));
    bResult = FALSE;
    goto _exit_point;
  }

  while (1)
  {
    ++nINILine;
    if (EndOfSection(pINIFile, nINILine))
      break;
    EnterDocItem(pINIFile, nINILine, &DocItem, bSilent, disp);
    if (DocItem.nType != -1)  /* Only first DocDesc in the .ini is processed */
    {
      memcpy(&DefaultDocOptions_Opt, &DocItem, sizeof(TDocType));
      DefaultDocOptions_Opt.sExt[0] = '\0';  /* Ignore ext */
      DefaultDocOptions_Opt.nType = tyPlain;  /* type plain */
      break;
    }
  }
  if (DocItem.nType == -1)  /* There were no entries in this section */
    goto _set_default_doc_options;

_exit_point:
  return bResult;
}

/* ************************************************************************
   Function: StoreDocType
   Description:
*/
static int StoreDocType(TSectionBuf *pSec, const TDocType *pDocType)
{
  if (!SectionPrintF(pSec,
    "%s = '%s', '%s', %d, %s, %s, %s, %s, %s, %s\n",
    sKey_Doc,
    pDocType->sExt, GetSyntaxTypeName(pDocType->nType),
    pDocType->nTabSize,
    GetOnOff(pDocType->bUseTabs),
    GetOnOff(pDocType->bOptimalFill),
    GetOnOff(pDocType->bAutoIndent),
    GetOnOff(pDocType->bBackspaceUnindent),
    GetOnOff(pDocType->bCursorThroughTabs),
    GetOnOff(pDocType->bWordWrap)))
    return 2;
  return 0;
}

static TDocType *pDocTypesSnapshot;

/* ************************************************************************
  Function: DocTypesSnapshot
  Description:
    Called at boot time to take a snapshot.
    On exit document types are compared against it and
    if changed are stored to wglob.ini
*/
void DocTypesSnapshot(void)
{
  pDocTypesSnapshot = s_alloc(sizeof(TDocType) * (DocTypesNumEntries() + 1));
  if (pDocTypesSnapshot == NULL)
    return;
  CopyDocumentTypeSet(pDocTypesSnapshot, DocumentTypes);
}

void DocTypeSnapshotDispose(void)
{
  if (pDocTypesSnapshot != NULL)
    s_free(pDocTypesSnapshot);
}

/* ************************************************************************
   Function: DocTypesSectionChanged
   Description:
*/
static BOOLEAN DocTypesSectionChanged(void)
{
  if (pDocTypesSnapshot == NULL)  /* Memory error? What a shame! */
    return TRUE;
  if (!CheckDocumentTypeSet(DocumentTypes_Opt, pDocTypesSnapshot))
    return TRUE;
  /* TODO: this below might be unnecessary! */
  if (!CompareDocumentType(&DefaultDocOptions, &DefaultDocOptions_Opt))
    return TRUE;
  return FALSE;
}

/* ************************************************************************
   Function: StoreDocTypes
   Description:
     Stores DocumentTypes_Opt.
     Stores DefaultDocOptions_Opt.
*/
static int StoreDocTypes(TFile *pINIFile, TSectionBuf *pSec, BOOLEAN bForce)
{
  TDocType *pDocumentItem;
  int nSectionPos;

  if (!DocTypesSectionChanged())
    if (!bForce)
      return 0;

  SectionReset(pSec);

  if (!SectionPrintF(pSec, "[%s]\n", sSection_DocTypeSet))
  {
_failprintf:
    SectionReset(pSec);
    return 2;
  }

  for (pDocumentItem = DocumentTypes_Opt; !IS_END_OF_DOC_LIST(pDocumentItem); ++pDocumentItem)
  {
    if (StoreDocType(pSec, pDocumentItem) != 0)
      goto _failprintf;
  }

  nSectionPos = RemoveSection(pINIFile, sSection_DocTypeSet);
  if (!SectionInsert(pINIFile, nSectionPos, pSec))
    goto _failprintf;

  /*
  Now sSection_DefaultDoc
  */
  SectionReset(pSec);

  if (!SectionPrintF(pSec, "[%s]\n", sSection_DefaultDoc))
    goto _failprintf;

  if (StoreDocType(pSec, &DefaultDocOptions_Opt) != 0)
    goto _failprintf;

  nSectionPos = RemoveSection(pINIFile, sSection_DefaultDoc);
  if (!SectionInsert(pINIFile, nSectionPos, pSec))
    goto _failprintf;

  return 0;
}

/* ************************************************************************
   Function: LoadAnouncementOptions
   Description:
     Loads last build anoounced and last tip number shown
     from the master INI file (wglob.ini)
*/
static void LoadAnouncementOptions(TFile *pINIFile, BOOLEAN bSilent, dispc_t *disp)
{
  int nINILine;
  int nErrorCode;
  char Key[MAX_KEY_NAME_LEN];
  char Val[MAX_VAL_LEN];
  int nVal;

  ASSERT(pINIFile != NULL);

  if ((nErrorCode = SearchSection(pINIFile, sSection_Global, &nINILine)) != 0)
  {
    if (nErrorCode != ERROR_NO_SUCH_SECTION)  /* If no such section EndOfSection() will exit */
    {
      DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
    }
    return;
  }

  while (1)
  {
    ++nINILine;
    if (EndOfSection(pINIFile, nINILine))
      break;

    if ((nErrorCode = ParseINILine(GetLineText(pINIFile, nINILine), Key, Val)) != 0)
    {
      if (nErrorCode != ERROR_LINE_EMPTY)
        DisplayINIFileError(pINIFile, nINILine, nErrorCode, bSilent, disp);
      continue;
    }

    if (stricmp(Key, sKey_LastTip) == 0)
    {
      if (!ValStr(Val, &nVal, 10))
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_NUMBER, bSilent, disp);
      else
        nLastTip_Opt = nVal;
      continue;
    }

    if (stricmp(Key, sKey_LastBuild) == 0)
    {
      if (!ValStr(Val, &nVal, 10))
        DisplayINIFileError(pINIFile, nINILine, ERROR_INVALID_NUMBER, bSilent, disp);
      else
        nLastBuildAnounced_Opt = nVal;
      continue;
    }

    DisplayINIFileError(pINIFile, nINILine, ERROR_UNRECOGNIZED_KEY, bSilent, disp);
  }
}

/* ************************************************************************
   Function: GlobalSectionChanged
   Description:
     Checks whether the parameters in the [Global] section has changed.
   Returns: TRUE - changed, FALS - not_changed
*/
static BOOLEAN GlobalSectionChanged(void)
{
  if (nLastTip != nLastTip_Opt)
    return TRUE;
  if (nLastBuildAnounced != nLastBuildAnounced_Opt)
    return TRUE;
  return FALSE;  /* this section is not changed */
}

/* ************************************************************************
   Function: StoreAnouncementOptions
   Description:
*/
static int StoreAnouncementOptions(TFile *pINIFile, TSectionBuf *pSec)
{
  int nSectionPos;

  if (!GlobalSectionChanged())
    return 0;

  SectionReset(pSec);

  if (!SectionPrintF(pSec, "[%s]\n", sSection_Global))
  {
_failprintf:
    SectionReset(pSec);
    return 2;
  }

  if (!SectionPrintF(pSec, "%s = %d\n", sKey_LastTip, nLastTip))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %d\n", sKey_LastBuild, nLastBuildAnounced))
    goto _failprintf;

  nSectionPos = RemoveSection(pINIFile, sSection_Global);
  if (!SectionInsert(pINIFile, nSectionPos, pSec))
    goto _failprintf;

  return 0;
}

/* StoreScreenGeometry() is only for graphical envyronments */
#ifdef _NON_TEXT
/* ************************************************************************
   Function: StoreScreenGeometry
   Description:
     Loads the options from the master INI file (wglob.ini).
     [WindowsScreen] | [XScreen] may hold
     Pos=X,Y
     Size=W,H
     Maximized=Yes|No
     Font1=fontname
     Font1Size=Sz
     Font1Style=Style  ; Style is 4bytes hex [0|underscored|italic|bold]
     Font2Style=Style  ; Only for the case of having comments italic
     Font3Style=Style  ; Only for the case of underlining text

     fontname could be "name[, moredata]"
     FontName will be read until the end of the line of first ';' character
     "" respected, ofcourse
  Returns:
    0 -- section stored successfully
    2 -- Memory not enough for temp buffer
*/
static int StoreScreenGeometry(TFile *pINIFile,
  const char *sSection, TSectionBuf *pSec, dispc_t *disp)
{
  int nSectionPos;
  disp_wnd_param_t wnd;

  SectionReset(pSec);

  if (!SectionPrintF(pSec, "[%s]\n", sSection))
  {
_failprintf:
    SectionReset(pSec);
    return 2;
  }

  disp_wnd_get_param(disp, &wnd);
  if (!SectionPrintF(pSec, "%s = %d, %d\n", sKey_Pos, wnd.pos_x, wnd.pos_y))
    goto _failprintf;

  if (wnd.is_maximized)
  {
    if (!SectionPrintF(pSec, "%s = %d, %d\n", sKey_Size,
      wnd.height_before_maximize, wnd.height_before_maximize))
      goto _failprintf;
  }
  else
  {
    if (!SectionPrintF(pSec, "%s = %d, %d\n", sKey_Size, wnd.width, wnd.height))
      goto _failprintf;
  }

  if (!SectionPrintF(pSec, "%s = %s\n", sKey_Maximized, GetYesNo(wnd.is_maximized)))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = \"%s\"\n", sKey_Font1, wnd.font_name))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %d\n", sKey_Font1Sz, wnd.font_size))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %03x\n", sKey_Font1Style, wnd.font1_style))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %03x\n", sKey_Font2Style, wnd.font2_style))
    goto _failprintf;

  if (!SectionPrintF(pSec, "%s = %03x\n", sKey_Font3Style, wnd.font3_style))
    goto _failprintf;

  nSectionPos = RemoveSection(pINIFile, sSection);
  if (!SectionInsert(pINIFile, nSectionPos, pSec))
    goto _failprintf;

  return 0;
}
#endif /* NON_TEXT */

/* ************************************************************************
   Function: LoadScreenGeometry
   Description:
     Loads the options from the master INI file (wglob.ini).
     If there is no screen-geometry section in the .ini file, the
     editor must boot successfully with the default settings in gui_scr.c
*/
void LoadScreenGeometry(TFile *pINIFile, disp_wnd_param_t *wnd)
{
  const char *sSectionName;
  int nINILine;
  int nErrorCode;
  char Key[MAX_KEY_NAME_LEN];
  char Val[MAX_VAL_LEN];
  char *pPosX;
  char *pPosY;
  char *pWidth;
  char *pHeight;
  BOOLEAN bTooLong;
  int FontStyle;

  memset(wnd, 0, sizeof(disp_wnd_param_t));
  wnd->set_defaults = 1;  /* assume no data from outside */

  sSectionName = NULL;
  #ifdef _NON_TEXT
  #ifdef WIN32
  sSectionName = sSection_SGeomWin;
  #else
  sSectionName = sSection_SGeomX;
  #endif
  #endif

  if (sSectionName == NULL)
    return;  /* This platform has no screen geometry parameters section */

  ASSERT(pINIFile != NULL);

  if ((nErrorCode = SearchSection(pINIFile, sSectionName, &nINILine)) != 0)
    return;  /* Errors are only silent, screen is not up yet! */

  while (1)
  {
    ++nINILine;
    if (EndOfSection(pINIFile, nINILine))
      break;

    if ((nErrorCode = ParseINILine(GetLineText(pINIFile, nINILine), Key, Val)) != 0)
      continue;

    /*TRACE2("key: %s=%s\n", Key, Val);*/

    if (stricmp(Key, sKey_Pos) == 0)
    {
      pPosX = strtok(Val, ", ");
      pPosY = strtok(NULL, ", ");

      if (pPosX == NULL || pPosY == NULL)
        continue;  /* Skip the line silently, screen is not up yet! */

      if (!ValStr(pPosX, &wnd->pos_x, 10))
        continue;

      if (!ValStr(pPosY, &wnd->pos_y, 10))
        continue;
    }
    else if (stricmp(Key, sKey_Size) == 0)
    {
      pWidth = strtok(Val, ", ");
      pHeight = strtok(NULL, ", ");

      if (pWidth == NULL || pHeight == NULL)
        continue;  /* Skip the line silently, screen is not up yet! */

      if (!ValStr(pWidth, &wnd->width, 10))
        continue;

      if (!ValStr(pHeight, &wnd->height, 10))
        continue;
    }
    else if (stricmp(Key, sKey_Maximized) == 0)
    {
      DetectYesNo(Val, &wnd->is_maximized);
    }
    else if (stricmp(Key, sKey_Font1) == 0)
    {
      ExtractQuotedString(Val, wnd->font_name, "", MAX_FONT_NAME_LEN, &bTooLong);
    }
    else if (stricmp(Key, sKey_Font1Sz) == 0)
    {
      if (!ValStr(Val, &wnd->font_size, 10))
        continue;
    }
    else if (stricmp(Key, sKey_Font1Style) == 0)
    {
      if (!ValStr(Val, &FontStyle, 16))
        continue;
      wnd->font1_style = FontStyle;
    }
    else if (stricmp(Key, sKey_Font2Style) == 0)
    {
      if (!ValStr(Val, &FontStyle, 16))
        continue;
      wnd->font2_style = FontStyle;
    }
    else if (stricmp(Key, sKey_Font2Style) == 0)
    {
      if (!ValStr(Val, &FontStyle, 16))
        continue;
      wnd->font3_style = FontStyle;
    }
    else
      ;  /* Unrecognized key ... but no screen to report it into */
  }

  /* there was a geometry section in wglob.ini */
  wnd->set_defaults = 0;
}

/* ************************************************************************
   Function: LoadMasterOptions
   Description:
     Loads the options from the master INI file (wglob.ini).
     Copyies from the backup version of the options (*_Opt).
*/
void LoadMasterOptions(TFile *pINIFile, BOOLEAN bSilent, dispc_t *disp)
{
  LoadAnouncementOptions(pINIFile, bSilent, disp);
  LoadDocTypes(pINIFile, bSilent, disp);

  /* [Global] */
  nLastTip = nLastTip_Opt;
  nLastBuildAnounced = nLastBuildAnounced_Opt;

  /* [DocTypes] */
  CopyDocumentTypeSet(DocumentTypes, DocumentTypes_Opt);

  /* [DefaultDocOptions] */
  memcpy(&DefaultDocOptions, &DefaultDocOptions_Opt, sizeof(TDocType));

  if (pINIFile->bReadOnly)
    bDontStoreMasterINIFile = TRUE;
}

/* ************************************************************************
   Function: StoreMasterOptions
   Description:
     Updates the options of the master INI file (wglob.ini)

   1. Load options from the disk
   2. Check what sections differ and remove those section from the file in memory
   3. Store what is remaining in the INI file (wglob.ini)
   4. Append the contents of the sections that were changed.
*/
void StoreMasterOptions(BOOLEAN bForce, dispc_t *disp)
{
  TFile *pINIFile;
  TSectionBuf Sec;

  if (bDontStoreMasterINIFile)
  {
    ConsoleMessageProc(disp, NULL, MSG_ERROR | MSG_INFO, NULL, sMasterINIFileNotStored);
    return;
  }

  pINIFile = CreateINI(sMasterINIFileName);  /* Allocate an empty file structure */
  if (!OpenINI(pINIFile, disp))  /* Load the INI file contents */
    return;
  if (INIFileIsEmpty(pINIFile))
    bForce = TRUE;

  if (!SectionBegin(&Sec))
    goto _exit;
  if (StoreAnouncementOptions(pINIFile, &Sec) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  if (StoreDocTypes(pINIFile, &Sec, bForce) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */

  #ifdef _NON_TEXT
  #ifdef WIN32
  if (StoreScreenGeometry(pINIFile, sSection_SGeomWin, &Sec, disp) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  #else
  if (StoreScreenGeometry(pINIFile, sSection_SGeomX, &Sec) != 0)
    goto _exit2;  /* TODO: show no-memory at exit-code 2 */
  #endif
  #endif

_exit2:
  SectionDisposeBuf(&Sec);

_exit:
  StoreINI(pINIFile, disp);
  CloseINI(pINIFile);
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

