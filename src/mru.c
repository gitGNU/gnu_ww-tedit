/*

File: mru.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 12th November, 1998
Descrition:
  Most recently used (MRU) files list managing routines.

*/

#include "global.h"
#include "wlimits.h"
#include "path.h"
#include "memory.h"
#include "diag.h"
#include "mru.h"

#define INIT_MRULIST(pMRUList)  INITIALIZE_LIST_HEAD(&pMRUList->flist)

#define TOP_ITEM(pMRUList)  ((TMRUListItem *)(pMRUList->flist.Flink))

#define BOTTOM_ITEM(pMRUList)  ((TMRUListItem *)(pMRUList->flist.Blink))

#define	END_OF_MRULIST(pMRUList, pMRUListItem)\
  (END_OF_LIST(&pMRUList->flist, &pMRUListItem->flink))

#define NEXT_ITEM(pMRUListItem)  ((TMRUListItem *)(pMRUListItem->flink.Flink))

#define PREV_ITEM(pMRUListItem)  ((TMRUListItem *)(pMRUListItem->flink.Blink))

#define MRULIST_EMPTY(pMRUList)  (IS_LIST_EMPTY(&pMRUList->flist))

#define REMOVE_BOTTOM_ITEM(pMRUList)\
  (TMRUListItem *)REMOVE_TAIL_LIST(&pMRUList->flist)

#define	REMOVE_ITEM(pMRUItem)\
  REMOVE_ENTRY_LIST(&pMRUItem->flink)

#define INSERT_ITEM_ATTOP(pMRUList, pMRUItem)\
  INSERT_HEAD_LIST(&pMRUList->flist, &pMRUItem->flink)

/* ************************************************************************
   Function: InitMRUList
   Description:
     Initial setup of an empty MRU list.
*/
void InitMRUList(TMRUList *pMRUList, int nMaxNumberOfItems)
{
  ASSERT(pMRUList != NULL);
  ASSERT(nMaxNumberOfItems > 0);

  INIT_MRULIST(pMRUList);
  pMRUList->nNumberOfItems = 0;
  pMRUList->nMaxNumberOfItems = nMaxNumberOfItems;
}

/* ************************************************************************
   Function: SetMaxMRUItems
   Description:
     To change the maximal number of items stored in
     a MRU list after it was intially setup by InitMRUList().
*/
void SetMaxMRUItems(TMRUList *pMRUList, int nMaxNumberOfItems)
{
  ASSERT(pMRUList != NULL);
  ASSERT(nMaxNumberOfItems > 0);

  pMRUList->nMaxNumberOfItems = nMaxNumberOfItems;
}

/* ************************************************************************
   Function: DoneMRUList
   Description:
*/
void DoneMRUList(TMRUList *pMRUList)
{
  TMRUListItem *pMRUItem;

  ASSERT(pMRUList != NULL);

  while (!MRULIST_EMPTY(pMRUList))
  {
    pMRUItem = REMOVE_BOTTOM_ITEM(pMRUList);
    s_free(pMRUItem);
  }
}

/* ************************************************************************
   Function: SearchInMRUList
   Description:
     Searches for a specific MRU item based on parameters.
   On entry:
     sFileName - file name to search for
     nCopy - copy number (if file has more than one copy opened in memory)
   Returns:
     NULL if no item found or pointer to MRU item.
*/
TMRUListItem *SearchInMRUList(const TMRUList *pMRUList, const char *sFileName, int nCopy)
{
  TMRUListItem *pMRUItem;
  char sShrunkFileName[_MAX_PATH];

  ASSERT(pMRUList != NULL);
  ASSERT(sFileName != NULL);
  ASSERT(nCopy >= 0);

  if (MRULIST_EMPTY(pMRUList))
    return NULL;

  /* Strip path	if possible */
  ShrinkPath(sFileName, sShrunkFileName, 0, TRUE);

  pMRUItem = TOP_ITEM(pMRUList);
  while (!END_OF_MRULIST(pMRUList, pMRUItem))
  {
    if (filestrcmp(pMRUItem->sFileName, sShrunkFileName) == 0
      && pMRUItem->nCopy == nCopy)
      return (pMRUItem);
    pMRUItem = NEXT_ITEM(pMRUItem);
  }

  return NULL;  /* No such a file */
}

/* ************************************************************************
   Function: SearchInMRUListNumber
   Description:
     Get's the n'th file from the top of the MRU list.
     Second copy entries are not counted.
*/
TMRUListItem *SearchInMRUListNumber(const TMRUList *pMRUList, int nFile)
{
  TMRUListItem *pMRUItem;

  ASSERT(pMRUList != NULL);
  ASSERT(nFile >= 0);

  if (MRULIST_EMPTY(pMRUList))
    return NULL;

  pMRUItem = TOP_ITEM(pMRUList);
  while (!END_OF_MRULIST(pMRUList, pMRUItem))
  {
    if (nFile == 0 && pMRUItem->nCopy == 0)
      return pMRUItem;
    if (pMRUItem->nCopy == 0)
      nFile--;
    pMRUItem = NEXT_ITEM(pMRUItem);
  }

  return NULL;  /* No such a file */
}

/* ************************************************************************
   Function: AddFileToMRUList
   Description:
     Adds item to MRUList or updates existing item.
   On entry:
     bUpdate = TRUE directs updating instead of adding item to MRUList.
   NOTE:
   Q: What's the difference between adding and updating as the file will
   be in the list anyway?
   A: Update operation preserves the file place in MRU list. So when
   a file is opened it is added (on top of the list) but when is closed
   (or on program exit) it is updated only to be preserved its place in
   the list.
   NOTE2:
   The case when the list if full (nNumberOfItems == nMaxNumberOfItems):
   If number of files load in memory is greater than nMaxNumberOfItems
   it may happen that some opened files to be not present in MRU list
   and thus to be not reopened on next editor session.
*/
void AddFileToMRUList(TMRUList *pMRUList, const char *sFileName, int nCopy,
  BOOLEAN bClosed, BOOLEAN bCurrent, BOOLEAN bForceReadOnly,
  int nCol, int nRow, int nTopLine, BOOLEAN bUpdate)
{
  TMRUListItem *pMRUItem;
  char sShrunkFileName[_MAX_PATH];
  TMRUListItem *pMRUItem2;


  ASSERT(pMRUList != NULL);
  ASSERT(sFileName != NULL);
  ASSERT(nCopy >= 0);
  ASSERT(pMRUList->nMaxNumberOfItems > 0);

  /* Strip path	if possible */
  ShrinkPath(sFileName, sShrunkFileName, 0, TRUE);

  /*
  If pMRUList->nNumberOfItems reached the limit
  remove bottom file.
  */
  if (pMRUList->nNumberOfItems == pMRUList->nMaxNumberOfItems)
  {
    pMRUItem = REMOVE_BOTTOM_ITEM(pMRUList);
    s_free(pMRUItem);
    --pMRUList->nNumberOfItems;
  }

  pMRUItem = SearchInMRUList(pMRUList, sShrunkFileName, nCopy);

  if (pMRUItem != NULL)
  {
    if (bUpdate)
      goto _store_file_info;
    REMOVE_ITEM(pMRUItem);  /* Remove this entry to avoid duplication */
    s_free(pMRUItem);
    --pMRUList->nNumberOfItems;
  }
  pMRUItem = s_alloc(sizeof(TMRUListItem));
  INSERT_ITEM_ATTOP(pMRUList, pMRUItem);
  ++pMRUList->nNumberOfItems;
_store_file_info:
  strcpy(pMRUItem->sFileName, sShrunkFileName);
  pMRUItem->nCopy = nCopy;
  pMRUItem->bClosed = bClosed;
  pMRUItem->nRow = nRow;
  pMRUItem->nCol = nCol;
  pMRUItem->nTopLine = nTopLine;
  pMRUItem->bCurrent = bCurrent;
  pMRUItem->bForceReadOnly = bForceReadOnly;

  if (bCurrent)
  {
    /*
    Reset all other itesm having the flag bCurrent set.
    */
    pMRUItem2 = TOP_ITEM(pMRUList);
    while (!END_OF_MRULIST(pMRUList, pMRUItem2))
    {
      if (pMRUItem2 != pMRUItem)
        pMRUItem2->bCurrent = FALSE;
      pMRUItem2 = NEXT_ITEM(pMRUItem2);
    }
  }
}

/* ************************************************************************
   Function: MRUListForEach
   Description:
*/
void MRUListForEach(TMRUList *pMRUList,
  void (*Process)(TMRUListItem *pItem, void *pContext),
  BOOLEAN bUpward, void *pContext)
{
  TMRUListItem *pMRUItem;

  if (bUpward)
    pMRUItem = TOP_ITEM(pMRUList);
  else
    pMRUItem = BOTTOM_ITEM(pMRUList);

  while (!END_OF_MRULIST(pMRUList, pMRUItem))
  {
    Process(pMRUItem, pContext);

    if (bUpward)
      pMRUItem = NEXT_ITEM(pMRUItem);
    else
      pMRUItem = PREV_ITEM(pMRUItem);
  }
}

/* ************************************************************************
   Function: RemoveMRUItem
   Description:
*/
void RemoveMRUItem(TMRUList *pMRUList, const char *sFileName, int nCopy)
{
  TMRUListItem *pMRUItem;
  char sShrunkFileName[_MAX_PATH];

  ASSERT(pMRUList != NULL);
  ASSERT(sFileName != NULL);
  ASSERT(nCopy >= 0);
  ASSERT(pMRUList->nMaxNumberOfItems > 0);

  /* Strip path	if possible */
  ShrinkPath(sFileName, sShrunkFileName, 0, TRUE);

  pMRUItem = SearchInMRUList(pMRUList, sShrunkFileName, nCopy);

  if (pMRUItem != NULL)
  {
    REMOVE_ITEM(pMRUItem);
    s_free(pMRUItem);
    --pMRUList->nNumberOfItems;
  }
}

/* ************************************************************************
   Function: RemoveTopMRUItem
   Description:
*/
void RemoveTopMRUItem(TMRUList *pMRUList)
{
  TMRUListItem *pMRUItem;

  pMRUItem = TOP_ITEM(pMRUList);

  REMOVE_ITEM(pMRUItem);
  s_free(pMRUItem);
  --pMRUList->nNumberOfItems;
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

