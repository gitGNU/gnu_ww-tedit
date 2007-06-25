/*

File: wrkspace.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th November, 1998
Descrition:
  Organises work space files storage. This includes file-in-memory storage and
  MRU list storage.

*/

#include "global.h"
#include "wrkspace.h"
#include "memory.h"
#include "scr.h"
#include "l1opt.h"
#include "file2.h"

TFileList *pFilesInMemoryList;
TMRUList *pMRUFilesList;
THistory *pFileOpenHistory;
THistory *pCalculatorHistory;
THistory *pFindHistory;
THistory *pFindInFilesHistory;
TBlock *pClipboard;
TBookmarksSet stUserBookmarks;
TBookmarksSet stFindInFiles1;
TBookmarksSet stFindInFiles2;
TBookmarksSet stInfoHistory;  /* Info reader history */
TBookmarksSet stOutput;  /* log files, tools output, etc. */
TSearchContext stSearchContext;
TContextHelp stCtxHelp;
TContainer stRootContainer;
TContainer *pModalContainer;  /* overloads stRootContainer if not NULL */
TContainer *pCurrentContainer;
TContainer *pMessagesContainer;
BOOLEAN b_clipbrd_owner;

/* ************************************************************************
   Function: InitWorkspace
   Description:
     Allocation and initial setup of workspace storage lists.
*/
void InitWorkspace(void)
{
  pFilesInMemoryList = s_alloc(sizeof(TFileList));
  pMRUFilesList = s_alloc(sizeof(TMRUList));
  pFileOpenHistory = s_alloc(sizeof(THistory));
  pFindHistory = s_alloc(sizeof(THistory));
  pCalculatorHistory = s_alloc(sizeof(THistory));
  pFindInFilesHistory = s_alloc(sizeof(THistory));
  pClipboard = NULL;

  InitFileList(pFilesInMemoryList);
  InitMRUList(pMRUFilesList, MAX_MRU_FILES);
  InitHistory(pFileOpenHistory, MAX_HISTORY_ITEMS);
  InitHistory(pFindHistory, MAX_HISTORY_ITEMS);
  InitHistory(pCalculatorHistory, MAX_CALCULATOR_HISTORY_ITEMS);
  InitHistory(pFindInFilesHistory, MAX_FINDINFILES_HISTORY_ITEMS);

  InitBMList();
  InitBMSet(&stUserBookmarks, "Bookmarks");
  InitBMSet(&stFindInFiles1, "Find in Files 1");
  InitBMSet(&stFindInFiles2, "Find in Files 2");
  InitBMSet(&stInfoHistory, "Info reader history");
  InitBMSet(&stOutput, "Output");

  InitSearchContext(&stSearchContext);

  InitContextHelp(&stCtxHelp);

  ContainerInit(&stRootContainer, NULL, 0, 0, ScreenWidth, ScreenHeight - 1);
  pCurrentContainer = &stRootContainer;
  pMessagesContainer = NULL;
  b_clipbrd_owner = FALSE;
}

/* ************************************************************************
   Function: GetCurrentFile
   Description:
     Returns the current file.
	 Current file is the file at the top of the pFilesInMemoryList.
*/
TFile *GetCurrentFile(void)
{
  ASSERT(pFilesInMemoryList != NULL);

  if (GetNumberOfFiles() == 0)
    return NULL;
  return GetFileListTop(pFilesInMemoryList);
}

/* ************************************************************************
   Function: GetNumberOfFiles
   Description:
*/
int GetNumberOfFiles(void)
{
  ASSERT(pFilesInMemoryList != NULL);

  return pFilesInMemoryList->nNumberOfFiles;
}

/* ************************************************************************
   Function: DoneWorkspace
   Description:
     Disposes the workspace storage lists from the heap.
*/
void DoneWorkspace(void)
{
  RemoveLastFile(pFilesInMemoryList, pMRUFilesList);
  DoneFileList(pFilesInMemoryList);
  DoneMRUList(pMRUFilesList);
  DoneHistory(pFileOpenHistory);
  DoneHistory(pFindHistory);
  DoneHistory(pCalculatorHistory);
  DoneHistory(pFindInFilesHistory);
  if (pClipboard != NULL)
    DisposeABlock(&pClipboard);
  DoneBMSet(&stUserBookmarks);
  DoneBMSet(&stFindInFiles1);
  DoneBMSet(&stFindInFiles2);
  DoneBMSet(&stInfoHistory);
  DisposeBMList();

  DoneSearchContext(&stSearchContext);

  DoneContextHelp(&stCtxHelp);

  s_free(pFilesInMemoryList);
  s_free(pMRUFilesList);
  s_free(pFileOpenHistory);
  s_free(pFindHistory);
  s_free(pCalculatorHistory);
  s_free(pFindInFilesHistory);
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

