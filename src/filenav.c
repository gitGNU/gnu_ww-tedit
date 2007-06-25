/*

File: filenav.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 12th November, 1998
Descrition:
  Files-in-memory list managing routines.

*/

#include "global.h"
#include "memory.h"
#include "filenav.h"
#include "l1def.h"
#include "filecmd.h"  /* PrintString() and small terminal */
#include "diag.h"  /* DiagContinue() */
#include "searchf.h"  /* ApplyFuncColors() */
#include "contain.h"
#include "fview.h"
#include "wrkspace.h"

#define INIT_FILELIST(pFileList)  INITIALIZE_LIST_HEAD(&pFileList->flist)

#define TOP_FILE(pFileList)  ((TFileListItem *)(pFileList->flist.Flink))

#define	END_OF_FILELIST(pFileList, pFileListItem)\
  (END_OF_LIST(&pFileList->flist, &pFileListItem->flink))

#define NEXT_FILE(pFileListItem)  ((TFileListItem *)(pFileListItem->flink.Flink))

#define FILELIST_EMPTY(pFileList)  (IS_LIST_EMPTY(&pFileList->flist))

#define INSERT_FILE_ATTOP(pFileList, pFileListItem)\
  INSERT_HEAD_LIST(&pFileList->flist, &pFileItem->flink)

#define INSERT_FILE_ATBOTTOM(pFileList, pFileItem)\
  INSERT_TAIL_LIST(&pFileList->flist, &pFileItem->flink)

#define INSERT_AFTER_FILE(pFileItemAfter, pFileItem)\
  INSERT_HEAD_LIST(&pFileItemAfter->flink, &pFileItem->flink)

#define REMOVE_FILE(pFileItem)\
  REMOVE_ENTRY_LIST(&pFileItem->flink)

#define REMOVE_TOP_FILE(pFileList)\
  (TFileListItem *)REMOVE_HEAD_LIST(&pFileList->flist)

#define REMOVE_BOTTOM_FILE(pFileList)\
  (TFileListItem *)REMOVE_TAIL_LIST(&pFileList->flist)

#define FILE_LIST_MAGIC 0x5c
#define FILE_ITEM_MAGIC 0x5d

#ifdef _DEBUG
#define VALID_PFILELIST(pFileList) ((pFileList) != NULL && (pFileList)->MagicByte == FILE_LIST_MAGIC)
#define VALID_PFILEITEM(pFileItem) ((pFileItem) != NULL && (pFileItem)->MagicByte == FILE_ITEM_MAGIC)
#else
#define VALID_PFILELIST(pFileList)  (1)
#define VALID_PFILEITEM(pFileItem)  (1)
#endif

static TContainer *pSwapIntoContainer;  /* debug purposes */

/* ************************************************************************
   Function: InitFileList
   Description:
     Initial setup of an empty file list.
*/
void InitFileList(TFileList *pFileList)
{
  ASSERT(pFileList != NULL);

  INIT_FILELIST(pFileList);
  pFileList->nNumberOfFiles = 0;
  pFileList->nDepth = 0;
  #ifdef _DEBUG
  pFileList->MagicByte = FILE_LIST_MAGIC;
  #endif
}

/* ************************************************************************
   Function: DoneFileList
   Description:
*/
void DoneFileList(TFileList *pFileList)
{
  TFileListItem *pFileItem;

  ASSERT(pFileList != NULL);

  while (!FILELIST_EMPTY(pFileList))
  {
    pFileItem = REMOVE_TOP_FILE(pFileList);
    ASSERT(VALID_PFILEITEM(pFileItem));
    #ifdef _DEBUG
    pFileItem->MagicByte = 0;
    #endif
    s_free(pFileItem);
  }
  pFileList->MagicByte = 0;
}

/* ************************************************************************
   Function: CheckFileList
   Description:
     Checks file list for consistency
*/
static void CheckFileList(const TFileList *pFileList)
{
  TFileListItem *pFileItem;

  ASSERT(VALID_PFILELIST(pFileList));

  pFileItem = TOP_FILE(pFileList);
  while (!END_OF_FILELIST(pFileList, pFileItem))
  {
    ASSERT(VALID_PFILEITEM(pFileItem));
    pFileItem = NEXT_FILE(pFileItem);
  }
}

/* ************************************************************************
   Function: AddFileInFileList
   Description:
     Allocates a file and inserts it at the top of the list.
     The allocated file is initialized as an empty file.
     Searches for other copys of this file and sets proper nCopy number
     of newly allocated file.

     If sFileName is NULL the file meant to be an empty new file.
*/
TFile *AddFileInFileList(TFileList *pFileList, const char *sFileName)
{
  TFile *pFile;
  TFile *pFirstCopy;
  TFileListItem *pFileItem;
  int nCopy;
  BOOLEAN bNew;
  TContainer *pTopFileContainer;

  CheckFileList(pFileList);

  nCopy = 0;
  bNew = (sFileName == NULL);

  pTopFileContainer = NULL;
  if (!FILELIST_EMPTY(pFileList))
  {
    pFileItem = TOP_FILE(pFileList);
    pTopFileContainer = pFileItem->stView.pContainer;
  }

  if (!bNew)
  {
    pFirstCopy = NULL;
    pFileItem = TOP_FILE(pFileList);
    while (!END_OF_FILELIST(pFileList, pFileItem))
    {
      if (filestrcmp(sFileName, pFileItem->pFile->sFileName) == 0)
      {
        ++nCopy;
        if (pFirstCopy == NULL)
          pFirstCopy = pFileItem->pFile;
      }
      pFileItem = NEXT_FILE(pFileItem);
    }
  }

  pFile = s_alloc(sizeof(TFile));
  InitEmptyFile(pFile);
  pFileItem = s_alloc(sizeof(TFileListItem));
  #ifdef _DEBUG
  pFileItem->MagicByte = FILE_ITEM_MAGIC;
  #endif

  INSERT_FILE_ATTOP(pFileList, pFileItem);

  pFileItem->pFile = pFile;
  pFileItem->nFileNumber = pFileList->nNumberOfFiles++;
  if (bNew)
    strcpy(pFile->sFileName, sNoname);
  else
    strcpy(pFile->sFileName, sFileName);
  GetFullPath(pFile->sFileName);
  pFile->nCopy = nCopy;

  /*
  For all the copys in the memory load the cursor position of the
  first copy.
  */
  if (nCopy > 0)
  {
    pFile->nTopLine = pFirstCopy->nTopLine;
    pFile->nCol = pFirstCopy->nCol;
    pFile->nRow = pFirstCopy->nRow;
  }

  /*
  Prepare a view and insert the view in the place of the
  current view in the current container.
  */
  ViewInit(&pFileItem->stView);
  FileViewInit(&pFileItem->stView, pFileItem->pFile, &pFileItem->stFileView);

  if (pTopFileContainer != NULL)
    ContainerSetFocus(&stRootContainer, pTopFileContainer);
  ContainerSetView(pCurrentContainer, &pFileItem->stView);
  ContainerSetFocus(&stRootContainer, pCurrentContainer);

  /*
  Prepare an empty bookmarks set for the functions names
  */
  InitBMSet(&pFileItem->stFuncNames, "Function Names");
  BMSetPurgeFlag(&pFileItem->stFuncNames, TRUE);
  BMSetShowFileFlag(&pFileItem->stFuncNames, FALSE);

  /*
  Attach the _ExtraColorInterface_
  */
  pFileItem->stFileView.ExtraColorInterf.pExtraCtx =
    pFileItem->stFileView.FuncNameCtx;
  ASSERT(sizeof(TBMFindNextCtx) < sizeof(pFileItem->stFileView.FuncNameCtx));
  pFileItem->stFileView.ExtraColorInterf.pReserved1 = &pFileItem->stFuncNames;
  pFileItem->stFileView.ExtraColorInterf.pfnApplyExtraColors =
    (void*)ApplyFuncColors;
  pFileItem->stFileView.bColorInterfActivated = TRUE;

  /*
  Fill reverse mapping for TFile
  */
  pFile->pBMSetFuncNames = &pFileItem->stFuncNames;
  pFile->pBMFuncFindCtx = pFileItem->stFileView.FuncNameCtx;

  return pFile;
}

/* ************************************************************************
   Function: RemoveFile
   Description:
     Removes a file from top of the list. Then corrects remaining files
     numbers to be kept in consequtive manner.

     Search for other copys of the file in memory. If there are more
     than one copy of the same file in the filelist fixes the nCopy
     numbers of the other copys. If this is the first copy make one of
     the other to be not read-only.

     In fact it is necessary to be suncronized the file copy
     number in filelist and MRU list. In order to keep this module
     independent a call-back function is provided ProcessCopy()
     that should reflect the changes in the filelist. This function
     is called whenever a file copy number was decreased by one.
     So this call-back function should call the proper MRU list
     support routine to update the correspondent file entry.

     Returns pointer to the file which entry has jut been
     removed from the _pFileList_.
*/
TFile *RemoveFile(TFileList *pFileList, void (*ProcessCopy)(TFile *pFile, void *pContext), void *pContext)
{
  int nFileNumber;
  TFileListItem *pFileItem;
  TFileListItem *pTargetFile;
  BOOLEAN bSaveReadOnly;  /* If	first copy is removed */
  BOOLEAN bSingleCopy;
  TFile *pFile;
  TFile *pFirstCopy;

  CheckFileList(pFileList);
  ASSERT(!FILELIST_EMPTY(pFileList));
  ASSERT(ProcessCopy != NULL);

  pTargetFile = REMOVE_TOP_FILE(pFileList);  /* Remove top file */
  nFileNumber = pTargetFile->nFileNumber;
  --pFileList->nNumberOfFiles;

  pFirstCopy = NULL;  /* First copy needs consistent pFileList! */

  if (nFileNumber == pFileList->nNumberOfFiles)
    goto _remove_target;  /* Top file was the last file (most recent file loaded) */

  if (FILELIST_EMPTY(pFileList))
    goto _remove_target;

  pFileItem = TOP_FILE(pFileList);
  bSingleCopy =	TRUE;
  while (!END_OF_FILELIST(pFileList, pFileItem))
  {
    if (pFileItem->nFileNumber > nFileNumber)
      --pFileItem->nFileNumber;

    if (filestrcmp(pTargetFile->pFile->sFileName, pFileItem->pFile->sFileName) == 0)
    {
      bSingleCopy = FALSE;

      if (pFileItem->pFile->nCopy == 0)
        bSaveReadOnly = pFileItem->pFile->bForceReadOnly;
      else  /* Check the taget file of being first copy to get its bReadOnly flag */
        if (pTargetFile->pFile->nCopy == 0)
          bSaveReadOnly = pTargetFile->pFile->bForceReadOnly;

      if (pFileItem->pFile->nCopy > pTargetFile->pFile->nCopy)
      {
        --pFileItem->pFile->nCopy;
        if (pFileItem->pFile->nCopy > 0)
          ProcessCopy(pFileItem->pFile, pContext);
        else
        {
          ASSERT(pFileItem->pFile->nCopy == 0);
          pFirstCopy = pFileItem->pFile;
        }
      }
    }

    pFileItem = NEXT_FILE(pFileItem);
  }

  if (bSingleCopy)
  {
    pFirstCopy = NULL;
    goto _remove_target;
  }

  if (pTargetFile->pFile->nCopy	> 0)
    goto _remove_target;

  /*
  Here: there are more than one copy. The first copy is removed.
  Therefore: Correct bReadOnly flag of the file that become first copy file.
  */
  pFileItem = TOP_FILE(pFileList);
  while (!END_OF_FILELIST(pFileList, pFileItem))
  {
    if (filestrcmp(pTargetFile->pFile->sFileName, pFileItem->pFile->sFileName) == 0)
      if (pFileItem->pFile->nCopy == 0)
      {
        pFileItem->pFile->bForceReadOnly = bSaveReadOnly;
        break;
      }

    pFileItem = NEXT_FILE(pFileItem);
  }

_remove_target:
  /*
  Remove the file from the current container. Put the new top file inplace.
  */
  if (pTargetFile->stView.pContainer != NULL)  /* File is on the screen */
  {
    if (!pTargetFile->stView.bOnFocus)
      ContainerSetFocus(&stRootContainer, pTargetFile->stView.pContainer);
    ContainerRemoveView(pTargetFile->stView.pContainer);
  }
  if (pFileList->nNumberOfFiles > 0)  /* At least one file to set in container */
  {
    pFileItem = TOP_FILE(pFileList);
    ContainerSetView(pCurrentContainer, &pFileItem->stView);
    ContainerSetFocus(&stRootContainer, pCurrentContainer);
  }

  /*
  Dispose of any function name bookmarks
  */
  BMListDisposeBMSet(&pTargetFile->stFuncNames);
  DoneBMSet(&pTargetFile->stFuncNames);

  /*
  Dispose of the TFileListItem envelope itself
  */
  pFile = pTargetFile->pFile;
  #ifdef _DEBUG
  pTargetFile->MagicByte = 0;
  #endif
  s_free(pTargetFile);

  if (pFirstCopy)
    ProcessCopy(pFirstCopy, pContext);  /* pFileList has consistent consequtive file numbers */

  /*
  Return the pFile record for the caller where it will be properly disposed
  */
  return pFile;
}

/* ************************************************************************
   Function: GetFileListTop
   Description:
     Returns the top most file of filelist plus navigation depth.
     Q: What is this navigation depth used for.
     A: While Ctrl+Tab is pressed repetivly without releasing
     the Ctrl key we need to navigate among files but not changing
     the actual Z-Order. To chosse the file that is to be
     visible as temporary top file we walk from the actual top
     to a certain depth and return this as top-most file. Once the control
     is release ZeroFileListDepth() function should be called to
     move the file from this specific depth at the top and to
     zero the depth.
*/
TFile *GetFileListTop(const TFileList *pFileList)
{
  TFileListItem *pFileItem;
  int nDepth;

  CheckFileList(pFileList);
  ASSERT(!FILELIST_EMPTY(pFileList));

  pFileItem = TOP_FILE(pFileList);
  nDepth = pFileList->nDepth;
  ASSERT(nDepth >= 0);
  ASSERT(nDepth <= pFileList->nNumberOfFiles);

  if (nDepth > 0)  /* Swap operation is taking place */
  {
    /* Activate the container of the top file where the swap should
    be executed */
    if (!pFileItem->stView.bOnFocus)
      ContainerSetFocus(&stRootContainer, pFileItem->stView.pContainer);
  }

  while (nDepth > 0)
  {
    pFileItem = NEXT_FILE(pFileItem);
    ASSERT(VALID_PFILEITEM(pFileItem));
    --nDepth;
  }

  return pFileItem->pFile;
}

/* ************************************************************************
   Function: ZeroFileListDepth
   Description:
     This function is to be called at the end of navigation sequence
     to zero the depth and to select new file to stay on top of the list.
*/
void ZeroFileListDepth(TFileList *pFileList)
{
  TFileListItem *pFileItem;
  int nDepth;

  CheckFileList(pFileList);
  ASSERT(!FILELIST_EMPTY(pFileList));

  pFileItem = TOP_FILE(pFileList);
  nDepth = pFileList->nDepth;
  ASSERT(nDepth >= 0);
  ASSERT(nDepth <= pFileList->nNumberOfFiles);

  if (nDepth == 0)
    return;

  /* sanity chk: we can't change containers when nDepth > 0! */
  ASSERT(pSwapIntoContainer == pCurrentContainer);

  while (nDepth > 0)
  {
    pFileItem = NEXT_FILE(pFileItem);
    --nDepth;
  }

  REMOVE_FILE(pFileItem);
  INSERT_FILE_ATTOP(pFileList, pFileItem);
  pFileList->nDepth = 0;
  ContainerSetView(pCurrentContainer, &pFileItem->stView);
  ContainerSetFocus(&stRootContainer, pCurrentContainer);
}

/* ************************************************************************
   Function: SwapFile
   Description:
     Increases or decreases the depth that selects the
     visible file in the list.
   On entry:
     bTop -- determines the direction
   Details:
     This function is to be used to navigate forward or backward in
     the file list as consecutive calls will go through all the file
     list. This functionality is usualy attached to Ctrl+Tab (or
     Ctrl+Shift+Tab) when pressed to navigate without releasing Ctrl
     key.
*/
void SwapFile(TFileList *pFileList, BOOLEAN bTop)
{
  CheckFileList(pFileList);
  ASSERT(!FILELIST_EMPTY(pFileList));

  if (pFileList->nNumberOfFiles == 1)
    return;  /* No operation for single file */

  if (bTop)
    --pFileList->nDepth;
  else
    ++pFileList->nDepth;

  pSwapIntoContainer = pCurrentContainer;

  /* Wrap the depth */
  if (pFileList->nDepth < 0)
    pFileList->nDepth = pFileList->nNumberOfFiles - 1;
  if (pFileList->nDepth == pFileList->nNumberOfFiles)
    pFileList->nDepth = 0;
}

/* ************************************************************************
   Function: GetTopFileNumber
   Description:
     Returns the number of the file that is on the top
     of the list.
*/
int GetTopFileNumber(TFileList *pFileList)
{
  TFileListItem *pFileItem;

  CheckFileList(pFileList);
  ASSERT(!FILELIST_EMPTY(pFileList));

  pFileItem = TOP_FILE(pFileList);
  return pFileItem->nFileNumber;
}

/* ************************************************************************
   Function: SetTopFileByLoadNumber
   Description:
     Moves a specific file on top of the list based on its consequtive
     number.
*/
void SetTopFileByLoadNumber(TFileList *pFileList, int nFile)
{
  TFileListItem *pFileItem;
  TContainer *pTopFileContainer;

  CheckFileList(pFileList);
  ASSERT(nFile < pFileList->nNumberOfFiles);

  pFileItem = TOP_FILE(pFileList);

  pTopFileContainer = NULL;
  pTopFileContainer = pFileItem->stView.pContainer;

  while (!END_OF_FILELIST(pFileList, pFileItem))
  {
    if (pFileItem->nFileNumber == nFile)
    {
      REMOVE_FILE(pFileItem);
      INSERT_FILE_ATTOP(pFileList, pFileItem);
      if (pTopFileContainer != NULL)
        ContainerSetFocus(&stRootContainer, pTopFileContainer);
      ContainerSetView(pCurrentContainer, &pFileItem->stView);
      ContainerSetFocus(&stRootContainer, pCurrentContainer);
      /* plot the new file on the screen */
      ContainerHandleEvent(MSG_INVALIDATE_SCR, pCurrentContainer, NULL);
      return;
    }
    pFileItem = NEXT_FILE(pFileItem);
  }
  ASSERT(0);
}

/* ************************************************************************
   Function: SetTopFileByZOrder
   Description:
     Moves a specific file on top of the list based on its z-order
     position.
*/
void SetTopFileByZOrder(TFileList *pFileList, int nDepthPos)
{
  TFileListItem *pFileItem;
  TContainer *pTopFileContainer;

  CheckFileList(pFileList);
  ASSERT(nDepthPos < pFileList->nNumberOfFiles);

  pFileItem = TOP_FILE(pFileList);

  pTopFileContainer = NULL;
  pTopFileContainer = pFileItem->stView.pContainer;

  while (!END_OF_FILELIST(pFileList, pFileItem))
  {
    if (nDepthPos == 0)
    {
      REMOVE_FILE(pFileItem);
      INSERT_FILE_ATTOP(pFileList, pFileItem);
      if (pTopFileContainer != NULL)
        ContainerSetFocus(&stRootContainer, pTopFileContainer);
      ContainerSetView(pCurrentContainer, &pFileItem->stView);
      ContainerSetFocus(&stRootContainer, pCurrentContainer);
      /* plot the new file on the screen */
      ContainerHandleEvent(MSG_INVALIDATE_SCR, pCurrentContainer, NULL);
      return;
    }
    --nDepthPos;
    pFileItem = NEXT_FILE(pFileItem);
  }
  ASSERT(0);
}

/* ************************************************************************
   Function: SearchFileList
   Description:
     Checks whether a particular file is present in the file list.
     Returns the consequtive file number of the file if present, otherwise
     returns -1.
*/
int SearchFileList(const TFileList *pFileList, const char *sFileName, int nCopy)
{
  TFileListItem *pFileItem;
  char sFullFileName[_MAX_PATH];

  CheckFileList(pFileList);
  ASSERT(sFileName != NULL);
  ASSERT(nCopy >= 0);

  strcpy(sFullFileName, sFileName);
  GetFullPath(sFullFileName);

  pFileItem = TOP_FILE(pFileList);
  while (!END_OF_FILELIST(pFileList, pFileItem))
  {
    if (filestrcmp(pFileItem->pFile->sFileName, sFullFileName) == 0 &&
      pFileItem->pFile->nCopy == nCopy)
      return pFileItem->nFileNumber;
    pFileItem = NEXT_FILE(pFileItem);
  }

  return -1;  /* Specific file is not found */
}

/* ************************************************************************
   Function: SearchFileList
   Description:
     Checks whether a particular file is present in the file list.
     Returns pointer to the file structure or NULL if there's no such file
     present.
*/
TFile *SearchFileListByID(const TFileList *pFileList, int nID)
{
  TFileListItem *pFileItem;

  CheckFileList(pFileList);

  pFileItem = TOP_FILE(pFileList);
  while (!END_OF_FILELIST(pFileList, pFileItem))
  {
    if (pFileItem->pFile->nID == nID)
      return pFileItem->pFile;
    pFileItem = NEXT_FILE(pFileItem);
  }

  return NULL;  /* Specific file is not found */
}

/* ************************************************************************
   Function: FileListForEach
   Description:
     Process() will be invoked for each file in the file list. The
     files will appear in their consequtive number order or Z order
     depending on bConsequtiveOrder parameter.
     If Process() returns FALSE the iteration will be terminated.
*/
void FileListForEach(const TFileList *pFileList, BOOLEAN (*Process)(TFile *pFile, void *pContext),
  BOOLEAN bConsequtiveOrder, void *pContext)
{
  TFileListItem *pFileItem;
  int i;

  CheckFileList(pFileList);
  ASSERT(Process != NULL);

  if (bConsequtiveOrder)
  {
    /*
    Walk files by their consequtive number
    */
    for (i = 0; i < pFileList->nNumberOfFiles; ++i)
    {
      pFileItem = TOP_FILE(pFileList);
      while (!END_OF_FILELIST(pFileList, pFileItem))
      {
        if (pFileItem->nFileNumber == i)
        {
          if (!Process(pFileItem->pFile, pContext))
            return;
          goto _process_next;
        }
        pFileItem = NEXT_FILE(pFileItem);
      }
      TRACE1("for file #%d\n", i);
      TRACE1("pFileList->nNumberOfFiles %d\n", pFileList->nNumberOfFiles);
      ASSERT(0);  /* File with improper consequtive number */
_process_next:
      ;  /* end of *for* loop */
    }
  }
  else
  {
    /*
    Walk files by Z order
    */
    pFileItem = TOP_FILE(pFileList);
    while (!END_OF_FILELIST(pFileList, pFileItem))
    {
      Process(pFileItem->pFile, pContext);
      pFileItem = NEXT_FILE(pFileItem);
    }
  }
}

/* ************************************************************************
   Function: FileListFindFView
   Description:
     Returns TFileView that contains pFile.
*/
TView *FileListFindFView(const TFileList *pFileList, TFile *pFile)
{
  TFileListItem *pFileItem;

  CheckFileList(pFileList);

  pFileItem = TOP_FILE(pFileList);
  while (!END_OF_FILELIST(pFileList, pFileItem))
  {
    if (pFileItem->pFile == pFile)
      return &pFileItem->stView;
    pFileItem = NEXT_FILE(pFileItem);
  }
  return NULL;
}

/* ************************************************************************
   Function: GetFuncNamesBMSet
   Description:
     Returns TBookmarksSet that contains the function names of this file.
*/
TBookmarksSet *GetFuncNamesBMSet(const TFileList *pFileList, TFile *pFile)
{
  TFileListItem *pFileItem;

  CheckFileList(pFileList);

  pFileItem = TOP_FILE(pFileList);
  while (!END_OF_FILELIST(pFileList, pFileItem))
  {
    if (pFileItem->pFile == pFile)
      return &pFileItem->stFuncNames;
    pFileItem = NEXT_FILE(pFileItem);
  }
  return NULL;
}

/* ************************************************************************
   Function: DumpFileList
   Description:
     Dumps a specified TFileList.
*/
void DumpFileList(const TFileList *pFileList)
{
  TFileListItem *pFileItem;
  int i;

  PrintString("FileList (%p)\n", pFileList);

  CheckFileList(pFileList);

  if (FILELIST_EMPTY(pFileList))
  {
    PrintString("empty\n");
    return;
  }

  PrintString("NumberOfFiles: %d\n", pFileList->nNumberOfFiles);

  pFileItem = TOP_FILE(pFileList);
  i = 1;
  while (!END_OF_FILELIST(pFileList, pFileItem))
  {
    PrintString("fileitem (%p), #%d, %s\n",
      pFileItem, pFileItem->nFileNumber, pFileItem->pFile->sTitle);
    if (!DiagContinue(i + 2))
      break;
    ++i;

    pFileItem = NEXT_FILE(pFileItem);
  }
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

