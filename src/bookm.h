/*

File: bookm.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 30th May, 2000
Descrition:
  Bookmarks manipulation functions.
  Bookmarks are used in FindInFiles, Output Parser, User Bookmarks,
  Location History and wherever a set of locations should be generated,
  navigated, stored, restored or maintained while files in memory
  are edited.

*/

#ifndef BOOKM_H
#define BOOKM_H

#include "file.h"
#include "fview.h"
#include "pageheap.h"
#include "contain.h"

/*
*** Global bookmark structures ***
All the bookmarks currently loaded in memory are stored in
a TBookmarkList structure that holds all the bookmarks sorted by
filename and location. Each of the bookmarks is described in
a TMarkLocation structure. Every bookmark holds a pointer to
a TBookmarkSet. TBookmarkSet acts like a filter to extract bookmarks
from TBookmarkList in order to be maintained as a logical group -- like
FindInFiles, UserBookmarks OutputParser lines etc.
*/

/*
As there can be a lot of bookmarks that point in a single file
it would be practicle to store the file name only once.
TBookmarkFileNameEntry is an entry in a list that contains
all the file names that have bookmarks pointed inside their contents.
*/
typedef struct _BookmarkFileNameEntry
{
  TListEntry link;
  char sFileName[_MAX_PATH];
} TBookmarkFileNameEntry;

/*
TMarkLocation stores basic bookmark information. All the bookmarks
of all the files are contained in a single list and are sorted
regarding the file:line pair.
Instead of storing absolute values of the lines where they are pointing
at, an offset is stored. Thus the bookmarks list correctness is easily
maintained while lines are removed or added in an edit session.

psContent fiels can be stored in 2 distinctive ways. It can be stored
separately for every single mark as a single block in the heap. It can
be collected in chunks of 4K, these chunks are part of a linked list
attached to specific bookmarks set. First scenario is suitable for
user bookmarks. Second is suitable for FindInFiles or ExternalToolOutput
as in these cases we only add new psContent and we never intend to
remove single bookmark or to change the pertinent psContent.
*/
typedef struct _MarkLocation
{
  TListEntry bmlink;  /* Participates in the global list of bookmarks */
  #ifdef _DEBUG
  BYTE MagicByte;
  #define BOOKMARKLOK_MAGIC  0x55
  #endif
  int nOffset;
  int nCol;
  int nTopLine;  /* -1 -- indicates not_in_use, top line of the page otherwise */
  BOOLEAN bRemoved;  /* The bookmark has fallen in a deleted region */
  BOOLEAN bVisited;  /* The bookmark was selected at least once */
  BOOLEAN bLastAction;  /* Preferred cursor position when displaying */
  int nRenderIndent;
  void *pSet;
  TBookmarkFileNameEntry *pFileName;
  char *psContent;
  void *pExtraRec;
  WORD nOptions;
  BOOLEAN bIndicateOnScreen;  /* Destination line should be indicated */
  int nRow;  /* Used to precalculate rows when custom sorted order is required */
  void *pNext;  /* Used for bookmarks with a preset custom order */
} TMarkLocation;

#define BOOKM_STATIC 1  /* Collect the bookmarks contents in chunks of 4K */

/*
Bookmarks sets: FindInFiles, Output Parser, User Bookmarks,
  Location History or whatever later is added.
TFile Viewer field is a in-memory file that is generated based
on the bookmarks locations -- used in the viewer.
*/
typedef struct _BookmarksSet
{
  #ifdef _DEBUG
  BYTE MagicByte;
  #define BOOKMARKSSET_MAGIC  0x54
  #endif
  void *pFirst;  /* First, provided there is custom order */
  char sTitle[_MAX_PATH];  /* Short description of this set */
  char *psEndOfFile;  /* instead of <*** End Of File ***> */
  TFile Viewer;  /* This is passed to the small editor/viewer */
  void *pViewerInterface;  /* Most likely TSEInterface */
  BOOLEAN bShowFileName;  /* When rendering */
  BOOLEAN bSetIsSorted;
  BOOLEAN bPurge;  /* BMListUpdate() purges marks pointing deleted lines */
  TFileView stFileView;  /* Event handler */
  TView stView;  /* To be inserted in a container */
  TListRoot blist;  /* The bookmarks contents can be stored as a list of TBlocks */
  BOOLEAN bViewDirty;  /* Indicates if the view file should be regenerated */
  BOOLEAN (*pfnActivateBookmark)(TMarkLocation *pMark, int nRow, void *pContext);
  void *pActivateBookmarksCtx;
} TBookmarksSet;

/*
TBookmarksList: There should be only one instance of this stucture in the
workspace. All the bookmarks that are loaded or generated during an editor
session should be contained in this list.
*/
typedef struct _BookmarksList
{
  TListRoot bmlist;  /* of TMarkLocation Sorted: key is filename:line */
  TPagedHeap PgHeap;  /* All the bookmarks are allocated in paged heap */
  TListRoot flist;  /* of TBookmarkFileNameEntry */
} TBookmarksList;

#ifdef _DEBUG
#define VALID_PMARKLOCATION(pMarkLoc) ((pMarkLoc) != NULL && (pMarkLoc)->MagicByte == BOOKMARKLOK_MAGIC)
#define VALID_PBOOKMARKSSET(pMarksSet) ((pMarksSet) != NULL && (pMarksSet)->MagicByte == BOOKMARKSSET_MAGIC)
#else
#define VALID_PMARKLOCATION(pMarkLoc)  (1)
#define VALID_PBOOKMARKSSET(pMarksSet)  (1)
#endif

typedef struct _BMFindNextCtx
{
  #ifdef _DEBUG
  BYTE MagicByte;
  #define BMFINDCTX_MAGIC  0x5b
  #endif
  TBookmarksSet *pBMSet;
  BOOLEAN bInitialMarkIsSet;
  TMarkLocation *pInitialMark;
  int nInitialRow;
  int nInitialMarkNum;
  int nStartRow;
  int nMarkNum;
  int nCurMarkNum;
  int nCalcRow;
  TMarkLocation *pMark;
  TBookmarkFileNameEntry *pFileName;
  const char *pTargetFileName;
} TBMFindNextCtx;

#ifdef _DEBUG
#define VALID_PBMFINDCTX(pBMCtx) ((pBMCtx) != NULL && (pBMCtx)->MagicByte == BMFINDCTX_MAGIC)
#else
#define VALID_PBMFINDCTX(pBMCtx)  (1)
#endif

extern TBookmarksSet *pstLastBMSet;

void InitBMList(void);
void DisposeBMList(void);

void InitBMSet(TBookmarksSet *pBMSet, const char *sTitle);
void BMSetPurgeFlag(TBookmarksSet *pBMSet, BOOLEAN bPurge);
void BMSetShowFileFlag(TBookmarksSet *pBMSet, BOOLEAN bShowFileName);
void DoneBMSet(TBookmarksSet *pBMSet);
void BMSetActivateBookmark(TBookmarksSet *pBMSet, BOOLEAN bSorted, void *pContext);

BOOLEAN BMListInsert(const char *psFileName, int nRow,
  int nCol, int nTopLine, const char *psContent,
  const void *pExtraRec, int nExtraRecSz,
  TBookmarksSet *pSet, TMarkLocation **pDMark, WORD nOptions);
void BMListRemoveBookmark(TMarkLocation *pMark);
TMarkLocation *BMListCheck(const char *psFileName, int nRow, TBookmarksSet *pSet);
void BMListUpdate(TFile *pFile, int nRow, int nLinesInserted);
void BMListForEach(BOOLEAN (*pfnActionFunc)(TMarkLocation *pMark, int nRow, void *pContext),
  TBookmarksSet *pBMSet, BOOLEAN bSorted, void *pContext);
TMarkLocation *BMSetFindFirstBookmark(TBookmarksSet *pBMSet,
  const char *sDestFileName,
  int nStartRow, int nMarkNum, TBMFindNextCtx *pBMCtx, int *pnCalcRow);
TMarkLocation *BMSetFindNextBookmark(TBMFindNextCtx *pBMCtx, int *pnCalcRow);
BOOLEAN BMSetFindCtxGetActiveFlag(TBMFindNextCtx *pBMCtx);
void BMSetRevertToFirst(TBMFindNextCtx *pBMCtx);
void BMSetNewSearchMark(TBMFindNextCtx *pBMCtx, int nNewMarkNum);
void BMSetNewSearchRow(TBMFindNextCtx *pBMCtx, int nNewRow);

typedef struct _DisplayBookmCtx
{
  TBookmarksSet *pThisSet;
  int nPreferredLine;
} TDisplayBookmCtx;
BOOLEAN DisplayBookmarksFunc(TMarkLocation *pMark, int nRow, void *pContext);

void BMSetNewMsg(TBookmarksSet *pBMSet, char *sMsg);
BOOLEAN RenderBookmarks(TBookmarksSet *pBMSet);
void BookmarksInvalidate(TBookmarksSet *pBookmarks);

void BMListDisposeBMSet(TBookmarksSet *pBMSet);

#endif /* ifndef BOOKM_H */

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

