/*

File: mru.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 12th November, 1998
Descrition:
  Most recently used (MRU) files list managing routines.

*/

#ifndef MRU_H
#define MRU_H

#include "clist.h"
#include "maxpath.h"

typedef struct _TMRUList
{
  TListRoot flist;
  int nNumberOfItems;
  int nMaxNumberOfItems;
} TMRUList;

typedef struct _TMRUListItem
{
  TListEntry flink;

  char sFileName[_MAX_PATH];  /* File name and shrunk path */
  int nCopy;  /* There can be more than one file copy in memory */

  BOOLEAN bClosed;  /* The file was closed */
  BOOLEAN bCurrent;  /* The file was the current file on exit */
  BOOLEAN bForceReadOnly;  /* File is opened in read only mode */

  /* Cursor position */
  int nRow;  /* Position of the cursor: Row */
  int nCol;  /* Position of the cursor: Col */
  int nTopLine;  /* Exact page position */
} TMRUListItem;

void InitMRUList(TMRUList *pMRUList, int nMaxNumberOfItems);
void SetMaxMRUItems(TMRUList *pMRUList, int nMaxNumberOfItems);
void DoneMRUList(TMRUList *pMRUList);
TMRUListItem *SearchInMRUList(const TMRUList *pMRUList,
  const char *sFileName, int nCopy);
TMRUListItem *SearchInMRUListNumber(const TMRUList *pMRUList, int nFile);
void AddFileToMRUList(TMRUList *pMRUList, const char *sFileName, int nCopy,
  BOOLEAN bClosed, BOOLEAN bCurrent, BOOLEAN bForceReadOnly, 
  int nCol, int nRow, int nTopLine, BOOLEAN bUpdate);
void MRUListForEach(TMRUList *pMRUList, void (*Process)(TMRUListItem *pItem, void *pContext),
  BOOLEAN bUpward, void *pContext);
void RemoveMRUItem(TMRUList *pMRUList, const char *sFileName, int nCopy);
void RemoveTopMRUItem(TMRUList *pMRUList);

#endif	/* ifndef MRU_H */

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

