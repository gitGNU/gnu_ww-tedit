/*

File: history.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 14th December, 1998
Descrition:
  Input line history list maintaining functions.

*/

#include "global.h"
#include "memory.h"
#include "history.h"

#define INIT_HISTLIST(pHist)  INITIALIZE_LIST_HEAD(&pHist->hlist)

#define THIST_ENTRY  TListEntry

#define PHIST_TEXT(pHistLine) ((char *)(pHistLine + 1))

#define SIZEOF_HISTITEM(sLine)  (strlen(sLine) + 1 + sizeof(TListEntry))

#define TOP_ITEM(pHist)  ((THIST_ENTRY *)(pHist->hlist.Flink))

#define BOTTOM_ITEM(pHist)  ((THIST_ENTRY *)(pHist->hlist.Blink))

#define END_OF_HISTLIST(pHist, pHistItem)\
  (END_OF_LIST(&pHist->hlist, pHistItem))

#define	INSERT_ITEM_ATTOP(pHist, pHistItem)\
  INSERT_HEAD_LIST(&pHist->hlist, pHistItem)

#define NEXT_ITEM(pHistItem)  ((THIST_ENTRY *)(pHistItem->Flink))

#define PREV_ITEM(pHistItem)  ((THIST_ENTRY *)(pHistItem->Blink))

#define HISTLIST_EMPTY(pHist)  (IS_LIST_EMPTY(&pHist->hlist))

#define REMOVE_BOTTOM_ITEM(pHist)\
  (THIST_ENTRY *)REMOVE_TAIL_LIST(&pHist->hlist)

#define REMOVE_TOP_ITEM(pHist)\
  (THIST_ENTRY *)REMOVE_HEAD_LIST(&pHist->hlist)

#define REMOVE_ITEM(pHistItem)\
  REMOVE_ENTRY_LIST(pHistItem)

/* ************************************************************************
   Function: InitHistory
   Description:
     Initial setup of a history list.
*/
void InitHistory(THistory *pHist, int nMaxItems)
{
  ASSERT(pHist != NULL);

  INIT_HISTLIST(pHist);
  pHist->nMaxItems = nMaxItems;
}

/* ************************************************************************
   Function: SetMaxHistoryItems
   Description:
     Only to change the nMaxItems field after the history collection
     is initialized by InitHistory().
*/
void SetMaxHistoryItems(THistory *pHist, int nMaxItems)
{
  ASSERT(pHist != NULL);

  ASSERT(nMaxItems > 0);

  pHist->nMaxItems = nMaxItems;
}

/* ************************************************************************
   Function: _AddHistoryLine
   Description:
     Adds a line at the top of a history list.
     No checks for duplication or for list overflow (nMaxItems).
*/
static void _AddHistoryLine(THistory *pHist, const char *sLine)
{
  THIST_ENTRY *pHistLine;

  pHistLine = alloc(SIZEOF_HISTITEM(sLine));
  if (pHistLine == NULL)
    return;
  INSERT_ITEM_ATTOP(pHist, pHistLine);
  strcpy(PHIST_TEXT(pHistLine), sLine);
}

/* ************************************************************************
   Function: RemoveLastHistoryLine
   Description:
     Removes the line at the bottom of the history list.
*/
static void RemoveLastHistoryLine(THistory *pHist)
{
  TListEntry *pGhost;

  ASSERT(pHist != NULL);

  if (HISTLIST_EMPTY(pHist))
    return;

  pGhost = REMOVE_BOTTOM_ITEM(pHist);
  s_free(pGhost);
}

/* ************************************************************************
   Function: CountHistoryLines
   Description:
     Counts the number of items in a list.
*/
int CountHistoryLines(const THistory *pHist)
{
  int n;
  THIST_ENTRY *pItem;

  n = 0;
  pItem = TOP_ITEM(pHist);

  while (!END_OF_HISTLIST(pHist, pItem))
  {
    n++;
    pItem = NEXT_ITEM(pItem);
  }

  return n;
}

/* ************************************************************************
   Function: RemoveDuplication
   Description:
     Removes a duplication string from a history.
*/
static void RemoveDuplication(THistory *pHist, const char *sLine)
{
  THIST_ENTRY *pItem;
  THIST_ENTRY *pItem2;

  pItem = TOP_ITEM(pHist);
  while (!END_OF_HISTLIST(pHist, pItem))
  {
    pItem2 = pItem;
    pItem = NEXT_ITEM(pItem);

    if (stricmp(PHIST_TEXT(pItem2), sLine) == 0)
    {
      REMOVE_ITEM(pItem2);
      s_free(pItem2);
    }
  }
}

/* ************************************************************************
   Function: AddHistoryLine
   Description:
     Adds a line to a history.
     Removes the duplication lines.
     If the fill level threshold is reached, removes the oldest
     (from the tail) lines in the history.
*/
void AddHistoryLine(THistory *pHist, const char *sLine)
{
  ASSERT(pHist != NULL);
  ASSERT(sLine != NULL);
  ASSERT(strlen(sLine) > 0);
  ASSERT(pHist->nMaxItems > 0);
  ASSERT(CountHistoryLines(pHist) <= pHist->nMaxItems);

  if (CountHistoryLines(pHist) == pHist->nMaxItems)
    RemoveLastHistoryLine(pHist);

  RemoveDuplication(pHist, sLine);

  _AddHistoryLine(pHist, sLine);
}

/* ************************************************************************
   Function: EmptyHistory
   Description:
     Removes all the lines from a history.
*/
static void EmptyHistory(THistory *pHist)
{
  THIST_ENTRY *pLine;

  ASSERT(pHist != NULL);

  while (!HISTLIST_EMPTY(pHist))
  {
    pLine = REMOVE_TOP_ITEM(pHist);
    s_free(pLine);
  }
}

/* ************************************************************************
   Function: DoneHistory
   Description:
     Destroys a history.
*/
void DoneHistory(THistory *pHist)
{
  ASSERT(pHist != NULL);
  ASSERT(pHist->nMaxItems > 0);

  EmptyHistory(pHist);
  pHist->nMaxItems = 0;
}

#ifdef _DEBUG
/* ************************************************************************
   Function: DumpHistory
   Description:
     For control purposes.
*/
void DumpHistory(const THistory *pHist)
{
  THIST_ENTRY *pItem;

  pItem = TOP_ITEM(pHist);
  while (!END_OF_HISTLIST(pHist, pItem))
  {
    printf("%s\n", PHIST_TEXT(pItem));
    pItem = NEXT_ITEM(pItem);
  }
}
#endif

/* ************************************************************************
   Function: HistoryForEach
   Description:
     Iteration function.
     The call-back function Process() is called	for each of
     the history items.
     if bUpward is TRUE then the top-most item is passed first
     otherwise bottom-most is first.
*/
void HistoryForEach(const THistory *pHist,
  void (*Process)(const char *sHistLine, void *pContext), BOOLEAN bUpward,
  void *pContext)
{
  THIST_ENTRY *pItem;

  ASSERT(pHist != NULL);
  ASSERT(Process != NULL);

  if (bUpward)
    pItem = TOP_ITEM(pHist);
  else
    pItem = BOTTOM_ITEM(pHist);
  while (!END_OF_HISTLIST(pHist, pItem))
  {
    Process(PHIST_TEXT(pItem), pContext);
    if (bUpward)
      pItem = NEXT_ITEM(pItem);
    else
      pItem = PREV_ITEM(pItem);
  }
}

/* ************************************************************************
   Function: HistoryCheckUpdate
   Description:
     Replaces the last item if is the same like sOldText. This function
     can be called to replace the last history item whenever there are
     any incremental changes in the string.
*/
void HistoryCheckUpdate(THistory *pHist,
  const char *psOldText, const char *psNewText)
{
  THIST_ENTRY *pItem;
  THIST_ENTRY *pDummy;

  ASSERT(pHist != NULL);

  if (HISTLIST_EMPTY(pHist))
    return;

  pItem = TOP_ITEM(pHist);
  if (strcmp(PHIST_TEXT(pItem), psOldText) != 0)
    return;  /* No need of update */

  /* Replace the last line with the new content */
  pDummy = REMOVE_TOP_ITEM(pHist);
  s_free(pItem);
  AddHistoryLine(pHist, psNewText);
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

