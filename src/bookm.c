/*

File: bookm.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 30th May, 2000
Descrition:
  Bookmarks manipulation functions.
  Bookmarks are used in FindInFiles, External Tools Output Parser,
  User Bookmarks, Location History and whenever a set of locations
  should be generated, navigated, stored, restored or maintained while
  files in memory are edited.

*/

#include "global.h"
#include "memory.h"
#include "disp.h"
#include "bookm.h"
#include "block.h"
#include "l1def.h"
#include "file2.h"
#include "filecmd.h"
#include "heapg.h"  /* validate_heap */

#include "wrkspace.h"
#include "nav.h"

TBookmarksSet *pstLastBMSet = NULL;
static TBookmarksList BMList;  /* A single instance */
#ifdef _DEBUG
BOOLEAN bListInit = FALSE;
#define BLIST_INIT() bListInit
#else
#define BLIST_INIT() (1)
#endif

extern void MoveToFileView(void);

/* ************************************************************************
   Function: InitBMList
   Description:
     There is only one BookmarkList for an editor session. This function
     performs the initial setup of the list.
*/
void InitBMList(void)
{
  #ifdef _DEBUG
  bListInit = TRUE;
  #endif

  INITIALIZE_LIST_HEAD(&BMList.flist);
  INITIALIZE_LIST_HEAD(&BMList.bmlist);
  InitPagedHeap(&BMList.PgHeap, sizeof(TMarkLocation), 40, 0);
}

/* ************************************************************************
   Function: DisposeBMList
   Description:
     Disposes all the entries in the global bookmarks list.
*/
void DisposeBMList(void)
{
  TBookmarkFileNameEntry *pFileEntry;
  TMarkLocation *pMarkEntry;

  ASSERT(BLIST_INIT());

  /* Dispose all the filename entries */
  while (!IS_LIST_EMPTY(&BMList.flist))
  {
    pFileEntry = (TBookmarkFileNameEntry *)REMOVE_HEAD_LIST(&BMList.flist);
    s_free(pFileEntry);
  }

  /*
  Dispose all the bookmark entries.
  Unattach bookmarks from a files loaded in memory.
  */
  while (!IS_LIST_EMPTY(&BMList.bmlist))
  {
    pMarkEntry = (TMarkLocation *)REMOVE_HEAD_LIST(&BMList.bmlist);
    if ((pMarkEntry->nOptions & BOOKM_STATIC) == 0)
      s_free(pMarkEntry->psContent);
    ASSERT(BMList.PgHeap.pageLen == 40);
    FreePagedBlock(&BMList.PgHeap, pMarkEntry);
  }

  DonePagedHeap(&BMList.PgHeap);

  #ifdef _DEBUG
  bListInit = FALSE;
  #endif
}

/* ************************************************************************
   Function: InitBMSet
   Description:
*/
void InitBMSet(TBookmarksSet *pBMSet, const char *sTitle)
{
  ASSERT(BLIST_INIT());

  #ifdef _DEBUG
  pBMSet->MagicByte = BOOKMARKSSET_MAGIC;
  #endif
  InitEmptyFile(&pBMSet->Viewer);
  pBMSet->psEndOfFile = NULL;
  pBMSet->Viewer.bReadOnly = TRUE;
  pBMSet->bViewDirty = TRUE;
  pBMSet->pViewerInterface = NULL;
  pBMSet->pfnActivateBookmark = NULL;
  pBMSet->pActivateBookmarksCtx = NULL;
  pBMSet->pFirst = NULL;
  pBMSet->bShowFileName = TRUE;
  pBMSet->bSetIsSorted = FALSE;
  pBMSet->bPurge = FALSE;
  strcpy(pBMSet->sTitle, sTitle);
  INITIALIZE_LIST_HEAD(&pBMSet->blist);  /* list of TArrays of bookm contents */
  ViewInit(&pBMSet->stView);
  pBMSet->stView.bDockedView = TRUE;
  FileViewInit(&pBMSet->stView, &pBMSet->Viewer, &pBMSet->stFileView);
}

/* ************************************************************************
   Function: BMSetPurgeFlag
   Description:
     bPurge is used by BMListUpdate().
*/
void BMSetPurgeFlag(TBookmarksSet *pBMSet, BOOLEAN bPurge)
{
  ASSERT(BLIST_INIT());
  ASSERT(VALID_PBOOKMARKSSET(pBMSet));

  pBMSet->bPurge = bPurge;
}

/* ************************************************************************
   Function: BMSetShowFileNameFlag
   Description:
*/
void BMSetShowFileFlag(TBookmarksSet *pBMSet, BOOLEAN bShowFileName)
{
  ASSERT(BLIST_INIT());
  ASSERT(VALID_PBOOKMARKSSET(pBMSet));

  pBMSet->bShowFileName = bShowFileName;
}

/* ************************************************************************
   Function: IsBMSetOnScreen
   Description:
     TRUE - bmset is on screen
     FALSE - not on screen
*/
BOOLEAN IsBMSetOnScreen(TBookmarksSet *pBMSet)
{
  ASSERT(BLIST_INIT());
  ASSERT(VALID_PBOOKMARKSSET(pBMSet));

  return pBMSet->stView.pContainer != NULL;
}

/* ************************************************************************
   Function: DoneBMSet
   Description:
*/
void DoneBMSet(TBookmarksSet *pBMSet)
{
  extern void CloseTheDockedView(void);  /* hacky, no better way comes to mind */

  ASSERT(BLIST_INIT());
  ASSERT(VALID_PBOOKMARKSSET(pBMSet));

  if (pstLastBMSet == pBMSet)
  {
    if (IsBMSetOnScreen(pBMSet))
      CloseTheDockedView();
    pstLastBMSet = NULL;
  }
  DisposeFile(&pBMSet->Viewer);
  DisposeBlockList(&pBMSet->blist);
}

/* ************************************************************************
   Function: BMListRenderFlags
   Description:
*/
static void BMListRenderFlags(TBookmarksSet *pBMSet, BOOLEAN bRemoved)
{
  char *pLine;

  if (pBMSet->Viewer.nRow >= pBMSet->Viewer.nNumberOfLines)
    return;

  pLine = GetLineText(&pBMSet->Viewer, pBMSet->Viewer.nRow);
  if (pLine[0] == ' ')
    pLine[0] = '*';
  if (bRemoved)
    pLine[0] = '-';
  bRemoved = FALSE;
}

/* ************************************************************************
   Function: CheckActivateBookmarkFunc
   Description:
     A call back function called to activate a specific bookmark
     in the list.
*/
static BOOLEAN CheckActivateBookmarkFunc(TMarkLocation *pMark, int nRow, void *pContext)
{
  BOOLEAN bRemoveFlag;
  TBookmarksSet *pCtx;
  TBookmarksSet *pReallyTheSet;  /* what hack for this disp context! */

  pCtx = pContext;
  if (pMark->pFileName == NULL)
    return FALSE;
  MoveToFileView();  /* Move away from the bookmarks viewer */
  bRemoveFlag = CheckToRemoveBlankFile();
  pReallyTheSet = pMark->pSet;
  if (CheckActivateFile(pMark->pFileName->sFileName,
                        pFilesInMemoryList, pMRUFilesList, pReallyTheSet->disp))
  {
    GotoColRow(GetCurrentFile(), pMark->nCol, nRow);
    pMark->bVisited = TRUE;
  }
  RemoveBlankFile(bRemoveFlag, pReallyTheSet->disp);
  return FALSE;
}

typedef struct _ActivateCtx
{
  BOOLEAN (*pfnActivateBookmark)(TMarkLocation *pMark, int nRow, void *pContext);
  int nActivateBookmark;
  BOOLEAN bRemoved;
  void *pOuterContext;
} TActivateCtx;

static BOOLEAN ActivateBookmark(TMarkLocation *pMark, int nRow, void *pContext)
{
  TActivateCtx *pCtx;

  pCtx = pContext;
  if (pCtx->nActivateBookmark > 0)
  {
    --pCtx->nActivateBookmark;
    return TRUE;
  }
  if (pMark->bRemoved)
    pCtx->bRemoved = TRUE;
  pCtx->pfnActivateBookmark(pMark, nRow, pCtx->pOuterContext);
  return FALSE;
}

/* ************************************************************************
   Function: BMSetActivateBookmark
   Description:
     Supposed to be called from the CmdEnter() by the bookmark viewer
*/
void BMSetActivateBookmark(TBookmarksSet *pBMSet, BOOLEAN bSorted, void *pContext)
{
  TActivateCtx stCtx;

  stCtx.bRemoved = FALSE;
  stCtx.nActivateBookmark = pBMSet->Viewer.nRow;
  if (pBMSet->pfnActivateBookmark == NULL)  /* Use the usual function */
    stCtx.pfnActivateBookmark = CheckActivateBookmarkFunc;
  else  /* Override with specialized function */
    stCtx.pfnActivateBookmark = pBMSet->pfnActivateBookmark;
  stCtx.pOuterContext = pContext;
  BMListForEach(ActivateBookmark, pBMSet, bSorted, &stCtx);
  BMListRenderFlags(pBMSet, stCtx.bRemoved);
  BookmarksInvalidate(pBMSet);  /* Redraw on the screen */
}

/* ************************************************************************
   Function: BMListSearchFileName
   Description:
     Checks whether a specific file name is part of the bookmark list.
*/
static TBookmarkFileNameEntry *BMListSearchFileName(const char *psFileName)
{
  TBookmarkFileNameEntry *pFileNameEntry;

  ASSERT(BLIST_INIT());

  /* Check whether an entry with such a filename alredy exists in the list */
  pFileNameEntry = (TBookmarkFileNameEntry *)(BMList.flist.Flink);
  while (!END_OF_LIST(&BMList.flist, &pFileNameEntry->link.Flink))
  {
    if (filestrcmp(pFileNameEntry->sFileName, psFileName) == 0)
      return pFileNameEntry;
    pFileNameEntry = (TBookmarkFileNameEntry *)(pFileNameEntry->link.Flink);
  }

  return NULL;
}

/* ************************************************************************
   Function: BMListInsertFileName
   Description:
     Allocates a filename entry. Inserts the entry in the
     filename section of the global bookmarks list. Returns
     pointer to the entry. If an entry for this filename
     already exists a pointer to this entry is returned.
*/
static TBookmarkFileNameEntry *BMListInsertFileName(const char *psFileName)
{
  TBookmarkFileNameEntry *pFileNameEntry;

  ASSERT(psFileName != NULL);
  ASSERT(BLIST_INIT());

  pFileNameEntry = BMListSearchFileName(psFileName);

  if (pFileNameEntry != NULL)
    return pFileNameEntry;

  pFileNameEntry = s_alloc(sizeof(TBookmarkFileNameEntry));
  strcpy(pFileNameEntry->sFileName, psFileName);
  INSERT_HEAD_LIST(&BMList.flist, &pFileNameEntry->link);
  return pFileNameEntry;
}

/* ************************************************************************
   Function: BMListInsert
   Description:
     Allocates a bookmark entry. Inserts the entry in the
     global bookmarks list.
     Returns FALSE upon failure to allocate memory.
     If psFileName is NULL, this is only a message, not a real bookmark. An example
     of such a message would be a line from a compiler output
     the line that contains the compiler name and version, or in Find In Files
     the line that talks about what we specified for searching.
     If pMark is not NULL, receives the address of the inserted mark.
     Options:
     BOOKM_STATIC: There can be 2 options for the psContent field.
       For entities like FindInFiles and ExternalOutput the entire bookmark
       set is created at once and there is no further change in the psContent
       field. For these sets, the actual text is maintained in TFileBlocks
       structure. For the User Bookmarks, each single psContent is stored
       in a single heap block and DisposeBMList() is taking care for disposing
       the psContent pointer. In the case of the others DoneBMSet() is
       disposing the entire block(s) where all the psContents point into.
*/
BOOLEAN BMListInsert(const char *psFileName, int nRow,
  int nCol, int nTopLine, const char *psContent,
  const void *pExtraRec, int nExtraRecSz,
  TBookmarksSet *pSet, TMarkLocation **pDMark, WORD nOptions)
{
  TBookmarkFileNameEntry *pFileEntry;
  TMarkLocation *pMark;
  TMarkLocation *pNewMark;
  TMarkLocation *pNextMark;
  int nCalcRow;
  int nBlockSize;
  int nBlockFree;
  int nNewSize;
  int nLineLen;
  int nRecordLen;
  char *pLinePos;
  char *pBlock;

  ASSERT(BLIST_INIT());
  ASSERT(nRow >= 0);

  //TRACE1("BMListInsert: %s\n", psContent);

  pFileEntry = NULL;
  if (psFileName != NULL)
    pFileEntry = BMListInsertFileName(psFileName);
  ASSERT(BMList.PgHeap.pageLen == 40);
  pNewMark = AllocPagedBlock(&BMList.PgHeap);
  if (pNewMark == NULL)
    return FALSE;
  #ifdef _DEBUG
  pNewMark->MagicByte = BOOKMARKLOK_MAGIC;
  #endif
  pNewMark->nOffset = 0;
  pNewMark->nCol = nCol;
  pNewMark->nTopLine = nTopLine;
  pNewMark->bRemoved = FALSE;
  pNewMark->bVisited = FALSE;
  pNewMark->bLastAction = TRUE;  /* Try to display this mark on the screen */
  pNewMark->pFileName = pFileEntry;
  pNewMark->nRenderIndent = 0;
  pNewMark->psContent = NULL;
  pNewMark->pExtraRec = NULL;
  pNewMark->pSet = pSet;
  pNewMark->pNext = NULL;
  pNewMark->nOptions = nOptions;
  pSet->bViewDirty = TRUE;

  if (pSet->pFirst == NULL)
    pSet->pFirst = pNewMark;

  /*
  Global bookmarks list is sorted. We need to find the exact position
  where to insert the new entry. While this we need to calculate the
  offset to be stored in the nOffset field.
  */
  pMark = (TMarkLocation *)(BMList.bmlist.Flink);
  /* First: find the position where all entries for this file start */
  if (pFileEntry != NULL)
  {
    while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink.Flink))
    {
      if (pMark->pFileName == pFileEntry)
        goto _file_exists;
      pMark = (TMarkLocation *)(pMark->bmlink.Flink);
    }

    /*
    This is the first entry for this file, add the bookmark to appear
    in alphabetical order from filenames perspective.
    */
    pMark = (TMarkLocation *)(BMList.bmlist.Flink);
    while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink.Flink))
    {
      if (pMark->pFileName != NULL &&
        filestrcmp(psFileName, pMark->pFileName->sFileName) < 0)
        break;
      pMark = (TMarkLocation *)(pMark->bmlink.Flink);
    }
  }

  /* Find the exact position where this row should reside */
_file_exists:
  nCalcRow = 0;
  while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink.Flink))
  {
    if (pMark->pFileName != pFileEntry)
      break;  /* End of this particular file -- put the bookmark here */
    if (nCalcRow + pMark->nOffset >= nRow)
      break;  /* Put the new bookmark here! */
    nCalcRow += pMark->nOffset;
    pMark = (TMarkLocation *)(pMark->bmlink.Flink);
  }
  INSERT_TAIL_LIST(&pMark->bmlink, &pNewMark->bmlink);

  /* Calculate the nOffset field of the newly inserted bookmark */
  pNewMark->nOffset = nRow - nCalcRow;
  ASSERT(pNewMark->nOffset >= 0);
  if (!END_OF_LIST(&BMList.bmlist, pNewMark->bmlink.Flink))
  {
    pNextMark = (TMarkLocation *)(pNewMark->bmlink.Flink);
    /* Only if pointing in the same file */
    if (pNextMark->pFileName == pNewMark->pFileName)
      pNextMark->nOffset -= pNewMark->nOffset;
  }

  if (pDMark != NULL)
    *pDMark = pNewMark;

  /*
  In case of static bookmarks content we need to copy the content
  in a TFileBlLock (list of blocks in pSet->blist)
  */
  if (psContent == NULL)
    return TRUE;

  nLineLen = strlen(psContent);

  if ((nOptions & BOOKM_STATIC) == 0)
  {
    pNewMark->psContent = alloc(nLineLen + 1);
    if (pNewMark->psContent == NULL)
    {
      BMListRemoveBookmark(pNewMark);
      return FALSE;
    }
    strcpy(pNewMark->psContent, psContent);
    return TRUE;
  }

  /*
  Static contents -- data is gathered in a list of blocks (>=4096). We always
  add into the last block at the end of the list.
  */
  nBlockFree = 0;
  nLineLen = ALIGN_TO(nLineLen + 1, 16);
  nRecordLen = nLineLen + nExtraRecSz;
  nRecordLen = ALIGN_TO(nRecordLen, 16);
  if (!IS_LIST_EMPTY(&pSet->blist))
  {
    pBlock = GetLastBlock(&pSet->blist);
    nBlockFree = GetTBlockFree(pBlock);
  }
  if (nBlockFree < nRecordLen)
  {
    nNewSize = 4096;
    if (nRecordLen > nNewSize)
      nNewSize = nRecordLen;
    if (!AddBlock(&pSet->blist, nNewSize))
    {
      BMListRemoveBookmark(pNewMark);  /* Undo add-bookmark */
      return FALSE;  /* indicate failure */
    }
  }

  /* There is a block that can accomodate the new line */
  ASSERT(BMList.PgHeap.pageLen == 40);
  pBlock = GetLastBlock(&pSet->blist);
  nBlockSize = GetTBlockSize(pBlock);
  nBlockFree = GetTBlockFree(pBlock);
  ASSERT(nBlockFree <= nBlockSize);
  ASSERT(nBlockFree >= nRecordLen);
  pLinePos = &pBlock[nBlockSize - nBlockFree];
  if (pExtraRec != NULL)
  {
    memcpy(pLinePos, pExtraRec, nExtraRecSz);
    pNewMark->pExtraRec = pLinePos;
    pLinePos += nExtraRecSz;
  }
  strcpy(pLinePos, psContent);
  pNewMark->psContent = pLinePos;
  nBlockFree -= nRecordLen;
  SetTBlockFree(pBlock, nBlockFree);
  /* Update the TFileBlock reference counter (not in use in this case) */
  IncRef(pBlock, 1);
  ASSERT(BMList.PgHeap.pageLen == 40);
  return TRUE;
}

/* ************************************************************************
   Function: BMListRemoveBookmark
   Description:
     Removes bookmark from the list of bookmarks. Corrects the offset
     of next bookmark to makeup for the offset of that being removed.
*/
void BMListRemoveBookmark(TMarkLocation *pMark)
{
  TMarkLocation *pNextMark;

  //TRACE1("BMListRemoveBookmark: %s\n", pMark->psContent);
  /*
  Update the offset of the prev element
  If it is first, then update next one.
  */
  if (!END_OF_LIST(&BMList.bmlist, pMark->bmlink.Flink))
  {
    pNextMark = (TMarkLocation *)pMark->bmlink.Flink;
    if (pNextMark->pFileName == pMark->pFileName)
    {
      pNextMark->nOffset += pMark->nOffset;
      pNextMark->bLastAction = TRUE;
    }
  }

  /* Remove mark from list */
  REMOVE_ENTRY_LIST(&pMark->bmlink);

  ((TBookmarksSet *)(pMark->pSet))->bViewDirty = TRUE;
  if ((pMark->nOptions & BOOKM_STATIC) == 0)
    s_free(pMark->psContent);
  ASSERT(BMList.PgHeap.pageLen == 40);
  FreePagedBlock(&BMList.PgHeap, pMark);
}

/* ************************************************************************
   Function: BMListCheck
   Description:
     Cehcks whether there is a bookmark at a specific line.
*/
TMarkLocation *BMListCheck(const char *psFileName, int nRow, TBookmarksSet *pSet)
{
  int nCalcRow;
  TMarkLocation *pMark;
  TBookmarkFileNameEntry *pFileName;
  TBookmarkFileNameEntry *pTargetFileName;

  ASSERT(BLIST_INIT());
  ASSERT(psFileName != NULL);

  pTargetFileName = BMListSearchFileName(psFileName);
  if (pTargetFileName == NULL)  /* do we have any entries for psFileName? */
    return FALSE;

  /*
  There is a bookmark in this file. Approach to nRow trying to determine whether
  we have a bookmark at this particular line.
  */
  /* Second: find the exact position where this row should reside */
  nCalcRow = 0;
  pMark = (TMarkLocation *)(BMList.bmlist.Flink);
  pFileName = NULL;
  while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink))
  {
    if (pFileName != pMark->pFileName)
    {
      pFileName = pMark->pFileName;
      nCalcRow = 0;
    }
    nCalcRow += pMark->nOffset;
    if (pMark->pFileName == pTargetFileName)  /* we are on the target file */
    {
      if (nCalcRow == nRow)  /* Are we on the target line? */
      {
        if (pMark->pSet == pSet)
          return pMark;
      }
      if (nCalcRow > nRow)  /* Did we pass the target line? */
        return NULL;
    }
    pMark = (TMarkLocation *)(pMark->bmlink.Flink);
  }

  return NULL;
}

/* ************************************************************************
   Function: BMListUpdate
   Description:
     Whenever lines are inserted or deleted in a file, this function
     should be called to update the correct positions of the bookmarks.
     TODO: The function cashes the last call by the psFileName pointer.
*/
void BMListUpdate(TFile *pFile, int nRow, int nLinesInserted)
{
  TBookmarkFileNameEntry *pFileName;
  int nCalcRow;
  int nAfterLastBookm;
  TMarkLocation *pMark;
  TMarkLocation *pPrevMark;
  TBookmarksSet *pPrevMarkSet;

  ASSERT(BLIST_INIT());
  ASSERT(VALID_PFILE(pFile));

  pFileName = BMListSearchFileName(pFile->sFileName);
  if (pFileName == NULL)
    return;

  /*
  There is a bookmark in this file. Approach to nRow trying to determine whether
  we have a bookmark at this particular line.
  */
  /* Second: find the exact position where this row should reside */
  nCalcRow = 0;
  pMark = (TMarkLocation *)(BMList.bmlist.Flink);
  /*
  Find the position where the entries for this file start
  in the bookmark list
  */
  while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink))
  {
    if (pMark->pFileName == pFileName)
      break;
    pMark = (TMarkLocation *)(pMark->bmlink.Flink);  /* Next in list */
  }

  if (nLinesInserted > 0)
  {
    if (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink))
    {
      ASSERT(VALID_PMARKLOCATION(pMark));
      if (pMark->nOffset > nRow)
      {
        /* We need to update only the first mark */
        pMark->nOffset += nLinesInserted;
        goto _exit;
      }
    }
    while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink) &&
      pMark->pFileName == pFileName)
    {
      ASSERT(VALID_PMARKLOCATION(pMark));
      nCalcRow += pMark->nOffset;
      if (nCalcRow > nRow)
      {
        /* Correct the offsets */
        pMark->nOffset += nLinesInserted;
        goto _exit;
      }
      pMark = (TMarkLocation *)(pMark->bmlink.Flink);  /* Next in list */
    }
    return;
  }

  /*
  Lines removed
  3 cases.
  1. All the deleted lines are in front of the
  bookmarks for this file.
  2. We delete a region in which some bookmarks
  falls into.
  3. We need then to correct all the bookmarks
  in this region, plus the bookmark immediately following the
  region.
  */
  if (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink))
  {
    ASSERT(VALID_PMARKLOCATION(pMark));
    if (pMark->nOffset > (-nLinesInserted) + nRow)
    {
      /* We need to update only the first offset */
      pMark->nOffset -= (-nLinesInserted);
      goto _exit;
    }
  }

  /* Find the row at which the first bookmark
  overlaps with the deleted region */
  while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink) &&
    pMark->pFileName == pFileName)
  {
    ASSERT(VALID_PMARKLOCATION(pMark));
    nCalcRow += pMark->nOffset;
    if (nCalcRow >= nRow)
      break;
    pMark = (TMarkLocation *)(pMark->bmlink.Flink);  /* Next in list */
  }

  if (END_OF_LIST(&BMList.bmlist, &pMark->bmlink) ||
    pMark->pFileName != pFileName)
    return;  /* The deletion region is after the last bookmark */

  nAfterLastBookm = -nLinesInserted;
  if (nRow + (-nLinesInserted) > nCalcRow)
  {
    /* At least one bookmark falls in the deleted region */
    ASSERT(VALID_PMARKLOCATION(pMark));
    pMark->nOffset -= nCalcRow - nRow;
    pMark->bRemoved = TRUE;
    pMark->bLastAction = TRUE;
    ((TBookmarksSet *)(pMark->pSet))->bViewDirty = TRUE;
    while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink) &&
      pMark->pFileName == pFileName)
    {
      ASSERT(VALID_PMARKLOCATION(pMark));
      pMark->nCol = 0;
      pMark = (TMarkLocation *)(pMark->bmlink.Flink);  /* Next in list */
      if (END_OF_LIST(&BMList.bmlist, &pMark->bmlink))
        break;
      ASSERT(VALID_PMARKLOCATION(pMark));
      nAfterLastBookm = (nRow + (-nLinesInserted)) - nCalcRow;
      nCalcRow += pMark->nOffset;
      if (nCalcRow > nRow + (-nLinesInserted))
        break;
      pMark->nOffset = 0;
      pMark->bRemoved = TRUE;
      ((TBookmarksSet *)(pMark->pSet))->bViewDirty = TRUE;
    }
  }

  /* We need to correct the offset of the first
  bookmark  that is following the region that has been deleted */
  if (END_OF_LIST(&BMList.bmlist, &pMark->bmlink) ||
    pMark->pFileName != pFileName)
    return;
  ASSERT(VALID_PMARKLOCATION(pMark));
  pMark->nOffset -= nAfterLastBookm;
_exit:
  ((TBookmarksSet *)(pMark->pSet))->bViewDirty = TRUE;
  pMark->bLastAction = TRUE;

  /*
  Remove all marks marked bRemove=TRUE and belong to pSet->bPurge=TRUE
  */
  pMark = (TMarkLocation *)(BMList.bmlist.Flink);
  /*
  Find the position where the entries for this file start
  in the bookmark list
  */
  while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink))
  {
    ASSERT(VALID_PMARKLOCATION(pMark));
    if (pMark->pFileName == pFileName)
      break;
    pMark = (TMarkLocation *)(pMark->bmlink.Flink);  /* Next in list */
  }
  pPrevMark = pMark;
  pPrevMarkSet = pMark->pSet;
  while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink) &&
    pMark->pFileName == pFileName)
  {
    ASSERT(VALID_PMARKLOCATION(pMark));
    pMark = (TMarkLocation *)(pMark->bmlink.Flink);  /* Next in list */
    if (pPrevMark->bRemoved && pPrevMarkSet->bPurge)
    {
      /* check: we don't delete from a sorted set! */
      ASSERT(pPrevMarkSet->bSetIsSorted == FALSE);
      ASSERT(pPrevMark->pNext == NULL);
      BMListRemoveBookmark(pPrevMark);
    }
    pPrevMark = pMark;
    pPrevMarkSet = pMark->pSet;
  }
}

/* ************************************************************************
   Function: BMListForEach
   Description:
     In non-sorted mode (bSorted is FALSE) pfnActionFunc() is called
     for each element in the list in the way they are ordered (Filename,
     nRow).
     In sorted mode (bSorted is TRUE) pfnActionFunc() is called based
     on predetermined by pNext field order of the bookmarks.
     User Bookmarks work with bSorted set to FALSE. Find In Files
     should work with bSorted set to TRUE. But the must use case
     would be for example the errors and all kind of messages output
     from a compiler or any other external executable. We need to preserve
     the original order of appearence!
*/
void BMListForEach(BOOLEAN (*pfnActionFunc)(TMarkLocation *pMark, int nRow, void *pContext),
  TBookmarksSet *pBMSet, BOOLEAN bSorted, void *pContext)
{
  TMarkLocation *pMark;
  int nRow;
  TBMFindNextCtx BMCtx;

  /* It is now implemented with FindFirst/FindNext */
  pMark = BMSetFindFirstBookmark(pBMSet, NULL, 0, 0, &BMCtx, &nRow);
  while (pMark != NULL)
  {
    ASSERT(VALID_PMARKLOCATION(pMark));
    if (!pfnActionFunc(pMark, nRow, pContext))
      break;
    pMark = BMSetFindNextBookmark(&BMCtx, &nRow);
  }
}

/* ************************************************************************
   Function: BMSetFindFirstBookmark
   Description:
     Similar to BMListForEach.
*/
TMarkLocation *BMSetFindFirstBookmark(TBookmarksSet *pBMSet,
  const char *sDestFileName,
  int nStartRow, int nMarkNum, TBMFindNextCtx *pBMCtx, int *pnCalcRow)
{
  BOOLEAN bSorted;
  int nCalcRow;
  TMarkLocation *pMark;
  int nPrevRow;
  TMarkLocation *pPrevMark;
  TBookmarkFileNameEntry *pFileName;
  BOOLEAN bFound;

  #ifdef _DEBUG
  pBMCtx->MagicByte = BMFINDCTX_MAGIC;
  #endif

  pBMCtx->pBMSet = pBMSet;
  pBMCtx->bInitialMarkIsSet = FALSE;
  pBMCtx->pInitialMark = NULL;
  pBMCtx->nInitialRow = 0;
  pBMCtx->nStartRow = nStartRow;
  pBMCtx->nCalcRow = 0;
  pBMCtx->nMarkNum = nMarkNum;
  pBMCtx->nCurMarkNum = 0;
  pBMCtx->pFileName = NULL;
  pBMCtx->pMark = NULL;
  pBMCtx->pTargetFileName = sDestFileName;

  bSorted = FALSE;
  if (pBMCtx->pBMSet != NULL)
    bSorted = pBMCtx->pBMSet->bSetIsSorted;

  if (bSorted)
  {
    /*
    First calculate all the row positions (only once)
    */
    ASSERT(VALID_PBOOKMARKSSET(pBMCtx->pBMSet));
    pMark = (TMarkLocation *)(BMList.bmlist.Flink);
    pFileName = NULL;
    nCalcRow = 0;
    while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink))
    {
      if (pMark->pFileName != pFileName)
      {
        pFileName = pMark->pFileName;
        pBMCtx->pFileName = pFileName;
        nCalcRow = 0;
      }
      nCalcRow += pMark->nOffset;
      pMark->nRow = nCalcRow;
      pMark = (TMarkLocation *)(pMark->bmlink.Flink);
    }
    pBMCtx->pMark = pBMCtx->pBMSet->pFirst;
  }

  /*
  Specific file is the target. Find the first bookmark of the file.
  */
  if (sDestFileName != NULL)
  {
    pMark = (TMarkLocation *)(BMList.bmlist.Flink);
    pFileName = NULL;
    nCalcRow = 0;
    pPrevMark = NULL;
    nPrevRow = 0;
    bFound = FALSE;
    while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink))
    {
      ASSERT(VALID_PMARKLOCATION(pMark));
      if (pMark->pFileName != pFileName)
      {
        pFileName = pMark->pFileName;
        if (strcmp(pFileName->sFileName, sDestFileName) == 0)
        {
          /*
          pBMCtx->bInitialMarkIsSet = TRUE;
          pBMCtx->pInitialMark = pPrevMark;
          pBMCtx->nInitialRow = 0;
          pBMCtx->nInitialMarkNum = pBMCtx->nMarkNum;
          */
          /* imitate RevertToFirst */
          pBMCtx->pMark = pPrevMark;
          pBMCtx->nCalcRow = 0;
          pBMCtx->nCurMarkNum = pBMCtx->nMarkNum;
          pBMCtx->pFileName = NULL;
          bFound = TRUE;
          break;
        }
        nCalcRow = 0;
      }
      nPrevRow = nCalcRow;
      nCalcRow += pMark->nOffset;
      pMark->nRow = nCalcRow;
      pPrevMark = pMark;
      pMark = (TMarkLocation *)(pMark->bmlink.Flink);
    }
    if (!bFound)  /* no bookmarks for this file? */
      return FALSE;
  }

  return BMSetFindNextBookmark(pBMCtx, pnCalcRow);
}

/* ************************************************************************
   Function: BMSetFindNextBookmark
   Description:
     It walks while reaches the target.
   Target is one of:
     nStartRow -- current bookmark Row is grater than nStartRow
     nMarkNum  -- walk until at least nMarkNum marks pass
*/
TMarkLocation *BMSetFindNextBookmark(TBMFindNextCtx *pBMCtx, int *pnCalcRow)
{
  BOOLEAN bSorted;
  TMarkLocation *pMark;
  TMarkLocation *pPrevMark;
  int nPrevRow;

  ASSERT(VALID_PBMFINDCTX(pBMCtx));

  bSorted = FALSE;
  if (pBMCtx->pBMSet != NULL)
    bSorted = pBMCtx->pBMSet->bSetIsSorted;

  if (!bSorted)
  {
    while (!END_OF_LIST(&BMList.bmlist, &pBMCtx->pMark->bmlink))
    {
      pPrevMark = pBMCtx->pMark;
      if (pBMCtx->pMark == NULL)  /* first call? */
        pBMCtx->pMark = (TMarkLocation *)(BMList.bmlist.Flink);
      else  /* move a mark forward into the link list */
        pBMCtx->pMark = (TMarkLocation *)(pBMCtx->pMark->bmlink.Flink);
      if (END_OF_LIST(&BMList.bmlist, &pBMCtx->pMark->bmlink))
      {
        /* --no-more-messages-- ln? */
        if (pBMCtx->nMarkNum != pBMCtx->nCurMarkNum)
          ASSERT(pBMCtx->nMarkNum == 0);  /* Not searching by consequ num! */
        return NULL;
      }

      ASSERT(VALID_PMARKLOCATION(pBMCtx->pMark));
      if (pBMCtx->pMark->pFileName != pBMCtx->pFileName)
      {
        /*
        Crossing a file boundary in the global bookmarks list
        */
        if (pBMCtx->nMarkNum > 0 || pBMCtx->nStartRow > 0)
        {
          /* We are doing nMarkNum or nStartRow type of search */
          if (pBMCtx->pFileName != NULL)  /* at least one file passed? */
            if (pBMCtx->pTargetFileName != 0)  /* only first file is to be processed */
              return NULL;
          pBMCtx->nCalcRow = pBMCtx->nInitialRow;  /* our zero start base */
        }
        else  /* We are only enumerating the bookmarks */
          pBMCtx->nCalcRow = 0;
        pBMCtx->pFileName = pBMCtx->pMark->pFileName;
      }

      nPrevRow = pBMCtx->nCalcRow;
      pBMCtx->nCalcRow += pBMCtx->pMark->nOffset;
      *pnCalcRow = pBMCtx->nCalcRow;
      if (pBMCtx->pBMSet != NULL)
        if (pBMCtx->pBMSet != pBMCtx->pMark->pSet)
          continue;
      if (pBMCtx->nCalcRow < pBMCtx->nStartRow)  /* reach specific row? */
        continue;
      if (pBMCtx->nCurMarkNum != pBMCtx->nMarkNum)  /* reach specific mark? */
      {
        //TRACE2("%d: %s\n", pBMCtx->nCurMarkNum, pBMCtx->pMark->psContent);
        ++pBMCtx->nCurMarkNum;
        continue;
      }
      /* Store the first mark to be used by BMRevertToFirst() */
      if (!pBMCtx->bInitialMarkIsSet)
      {
        /* We walk at least a mark before reaching the first */
        pBMCtx->bInitialMarkIsSet = TRUE;
        pBMCtx->pInitialMark = pPrevMark;
        pBMCtx->nInitialRow = nPrevRow;
        pBMCtx->nInitialMarkNum = pBMCtx->nMarkNum;
      }
      return pBMCtx->pMark;
    }
    return NULL;
  }

  /*
  bSorted
  */
  while (pBMCtx->pMark != NULL)
  {
    *pnCalcRow = pBMCtx->pMark->nRow;
    pMark = pBMCtx->pMark;
    pBMCtx->pMark = (TMarkLocation *)(pMark->pNext);

    ASSERT(VALID_PMARKLOCATION(pMark));
    /* And all marks belong to same set */
    ASSERT(pBMCtx->pBMSet == pMark->pSet);

    if (pBMCtx->nCalcRow < pBMCtx->nStartRow)
      continue;
    ++pBMCtx->nCurMarkNum;
    if (pBMCtx->nCurMarkNum < pBMCtx->nMarkNum)
      continue;
    /* Store the first mark to be used by BMRevertToFirst() */
    if (pBMCtx->pInitialMark == NULL)
    {
      pBMCtx->pInitialMark = pMark;
      pBMCtx->nInitialRow = *pnCalcRow;
      pBMCtx->nInitialMarkNum = pBMCtx->nMarkNum;
    }
    return pMark;
  }
  return NULL;
}

/* ************************************************************************
   Function: BMSetFindCtxGetActiveFlag
   Description:
*/
BOOLEAN BMSetFindCtxGetActiveFlag(TBMFindNextCtx *pBMCtx)
{
  return pBMCtx->bInitialMarkIsSet;
}

/* ************************************************************************
   Function: BMSetRevertToFirst
   Description:
     BMSetRevertToFirst() is preferable against BMSetFindFirstBookmark()
     because is sets to the first mark after nStartRow instead
     of starting from 0.
*/
void BMSetRevertToFirst(TBMFindNextCtx *pBMCtx)
{
  ASSERT(VALID_PBMFINDCTX(pBMCtx));
  ASSERT(pBMCtx->bInitialMarkIsSet == TRUE);
  pBMCtx->pMark = pBMCtx->pInitialMark;
  pBMCtx->nCalcRow = pBMCtx->nInitialRow;
  pBMCtx->nCurMarkNum = pBMCtx->nInitialMarkNum;
  pBMCtx->pFileName = NULL;
}

/* ************************************************************************
   Function: BMSetNewSearchMark
   Description:
     BMSetFindNextBookmark() uses initial condition from BMSetFindFirst().
     We can use the cached calculation of BMSetFindFirst() by calling
     BMSetRevertToFirst(). Then we can set new search criteria by
     calling BMSetNewSearchMark().
*/
void BMSetNewSearchMark(TBMFindNextCtx *pBMCtx, int nNewMarkNum)
{
  ASSERT(VALID_PBMFINDCTX(pBMCtx));
  ASSERT(pBMCtx->bInitialMarkIsSet == TRUE);
  pBMCtx->nMarkNum = nNewMarkNum;
}

/* ************************************************************************
   Function: BMSetNewSearchRow
   Description:
     BMSetFindNextBookmark() uses initial condition from BMSetFindFirst().
     We can use the cached calculation of BMSetFindFirst() by calling
     BMSetRevertToFirst(). Then we can set new search criteria by
     calling BMSetNewSearchRow().
*/
void BMSetNewSearchRow(TBMFindNextCtx *pBMCtx, int nNewRow)
{
  ASSERT(VALID_PBMFINDCTX(pBMCtx));
  ASSERT(pBMCtx->bInitialMarkIsSet == TRUE);
  pBMCtx->nStartRow = nNewRow;
}

/* ************************************************************************
   Function: DisplayBookmarksFunc
   Description:
     A call-back function invoked by BMListForEach.
     Prepares a human readable representation of the bookmarks in the list.
     Adds the output at the end of the Viewer file.
*/
BOOLEAN DisplayBookmarksFunc(TMarkLocation *pMark, int nRow, void *pContext)
{
  TDisplayBookmCtx *pCtx;
  char sShortName[_MAX_PATH];
  char *p;
  int nLen;
  char sALine[_MAX_PATH * 2 + 10];
  char *psContent;
  char sPrefix[2];
  BOOLEAN bResult;
  TBookmarksSet *pThisSet;
  int nRow2;

  ASSERT(VALID_PMARKLOCATION(pMark));

  pCtx = pContext;
  pThisSet = pCtx->pThisSet;

  if (pThisSet->psEndOfFile != NULL)  /* specialized message? */
    pThisSet->Viewer.sEndOfFile = pThisSet->psEndOfFile;
  else
    pThisSet->Viewer.sEndOfFile = (char *)sNoMoreMessages;

  psContent = "";
  if (pMark->psContent != NULL)
    psContent = pMark->psContent;
  nLen = strlen(psContent);

  /* Use a static buffer (sALine) or allocate in the heap if entry is too big */
  if (nLen > _MAX_PATH)
    p = s_alloc(nLen + _MAX_PATH + 10);
  else
    p = sALine;

  if (pMark->pFileName == NULL)
  {
    sprintf(p, "%s", psContent);
  }
  else
  {
    if (pThisSet->bShowFileName)
      ShrinkPath(pMark->pFileName->sFileName, sShortName, 0, TRUE);
    strcpy(sPrefix, " ");
    if (pMark->bVisited)
      strcpy(sPrefix, "*");
    if (pMark->bRemoved)
      strcpy(sPrefix, "-");
    if (pThisSet->bShowFileName)
      sprintf(p, "%s%s (%d): %s", sPrefix, sShortName, nRow + 1, psContent);
    else
      sprintf(p, "%s%d: %s", sPrefix, nRow + 1, psContent);
  }
  bResult = FALSE;
  if (AddCharLine(&pThisSet->Viewer, p))
    bResult = TRUE;

  pMark->nRenderIndent = strlen(p) - strlen(psContent);

  if (nLen > _MAX_PATH)
    s_free(p);

  if (pMark->bLastAction)  /* Hint of a preffered line? */
  {
    nRow2 = pThisSet->Viewer.nRow;  /* The line after the line of action */
    ASSERT(nRow2 > 0);
    --nRow2;  /* Now it has been corrected */
    pCtx->nPreferredLine = nRow2;
    pMark->bLastAction = FALSE;  /* Hint has been accepted */
  }

  return bResult;
}

/* ************************************************************************
   Function: BMSetNewMsg
   Description:
     Sets the message that is displayed at the end of the file
     Usually this should be "---no more messages---"
*/
void BMSetNewMsg(TBookmarksSet *pBMSet, char *sMsg)
{
  ASSERT(VALID_PBOOKMARKSSET(pBMSet));

  pBMSet->psEndOfFile = sMsg;
  pBMSet->bViewDirty = TRUE;  /* Redraw on the screen */
}

/* ************************************************************************
   Function: RenderBookmarks
   Description:
     Prepares a human readable representation of the bookmarks in the set.
*/
BOOLEAN RenderBookmarks(TBookmarksSet *pBMSet)
{
  TDisplayBookmCtx stCtx;
  int nTopLine;
  BOOLEAN bSorted;

  ASSERT(VALID_PBOOKMARKSSET(pBMSet));

  if (!pBMSet->bViewDirty)
    return TRUE;

  bSorted = pBMSet->bSetIsSorted;
  nTopLine = pBMSet->Viewer.nTopLine;
  DisposeFile(&pBMSet->Viewer);
  InitEmptyFile(&pBMSet->Viewer);
  if (pBMSet->psEndOfFile != NULL)  /* specialized message? */
    pBMSet->Viewer.sEndOfFile = pBMSet->psEndOfFile;
  else
    pBMSet->Viewer.sEndOfFile = (char *)sNoMoreMessages;
  pBMSet->Viewer.bReadOnly = TRUE;

  stCtx.pThisSet = pBMSet;
  stCtx.nPreferredLine = 0;
  BMListForEach(DisplayBookmarksFunc, pBMSet, bSorted, &stCtx);

  /* Got to the hinted line */
  pBMSet->Viewer.nTopLine = nTopLine;  /* otherwise we scroll to 0 */
  GotoColRow(&pBMSet->Viewer, 0, stCtx.nPreferredLine);

  if (bNoMemory)
    return FALSE;

  sprintf(pBMSet->Viewer.sTitle, "[%s]", pBMSet->sTitle);
  pBMSet->bViewDirty = FALSE;
  pBMSet->Viewer.bUpdatePage = TRUE;

  return TRUE;
}

/* ************************************************************************
   Function: BookmarksInvalidate
   Description:
     Invalidates specific bookmarks set.
     Finds whether the specific bookmarks are on the screen and
     redraws the container that holds the bookmarks set.
*/
void BookmarksInvalidate(TBookmarksSet *pBookmarks)
{
  TContainer *pCont;
  disp_event_t ev;

  ASSERT(VALID_PBOOKMARKSSET(pBookmarks));
  /* verify for valid pCurPos */
  if (pBookmarks->Viewer.nRow < pBookmarks->Viewer.nNumberOfLines)
    ASSERT(IS_VALID_CUR_POS(&pBookmarks->Viewer));

  pCont = FileViewFindContainer(&pBookmarks->stFileView);
  if (pCont == NULL)
    return;  /* nothing to invalidate, the bookmarks are not on the screen */
  RenderBookmarks(pBookmarks);

  disp_event_clear(&ev);
  ev.t.code = EVENT_USR;
  ev.t.user_msg_code = MSG_INVALIDATE_SCR;
  ContainerHandleEvent(pCont, &ev);
  pBookmarks->disp = pCont->disp;
}

/* ************************************************************************
   Function: BMListDisposeBMSet
   Description:
     Disposes all the bookmarks that belong to a specific set.
*/
void BMListDisposeBMSet(TBookmarksSet *pBMSet)
{
  TMarkLocation *pMark;
  TMarkLocation *pMarkNext;

  pMark = (TMarkLocation *)(BMList.bmlist.Flink);
  while (!END_OF_LIST(&BMList.bmlist, &pMark->bmlink))
  {
    pMarkNext = (TMarkLocation *)(pMark->bmlink.Flink);
    if (pMark->pSet == pBMSet)
      BMListRemoveBookmark(pMark);
    pMark = pMarkNext;
  }

  DisposeFile(&pBMSet->Viewer);
  InitEmptyFile(&pBMSet->Viewer);
  pBMSet->Viewer.bReadOnly = TRUE;
  pBMSet->pFirst = NULL;
  DisposeBlockList(&pBMSet->blist);
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

