/*

File: filenav.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 12th November, 1998
Descrition:
  Files-in-memory list managing routines.

*/

#ifndef FILENAV_H
#define FILENAV_H

#include "clist.h"
#include "file.h"
#include "bookm.h"

#include "fview.h"

typedef struct _TFileList
{
  #ifdef _DEBUG
  BYTE MagicByte;
  #endif
  TListRoot flist;
  int nNumberOfFiles;
  int nDepth;
} TFileList;

typedef struct _TFileListItem
{
  TListEntry flink;
  TFile *pFile;
  int nFileNumber;  /* Consequtive number assigned as the file is loaded */

  #ifdef _DEBUG
  BYTE MagicByte;
  #endif
  TView stView;
  TFileView stFileView;
  TBookmarksSet stFuncNames;
} TFileListItem;

void InitFileList(TFileList *pFileList);
void DoneFileList(TFileList *pFileList);
TFile *AddFileInFileList(TFileList *pFileList, const char *sFileName);
TFile *RemoveFile(TFileList *pFileList,
  void (*ProcessCopy)(TFile *pFile, void *pContext), void *pContext);
TFile *GetFileListTop(const TFileList *pFileList);
void SetSwapMarkerTop(TFileList *pFileList);
void SwapFile(TFileList *pFileList, BOOLEAN bTop);
void ZeroFileListDepth(TFileList *pFileList);
int GetTopFileNumber(TFileList *pFileList);
void SetTopFileByLoadNumber(TFileList *pFileList, int nFile);
void SetTopFileByZOrder(TFileList *pFileList, int nDepthPos);
int SearchFileList(const TFileList *pFileList, const char *sFileName, int nCopy);
TFile *SearchFileListByID(const TFileList *pFileList, int nID);
void FileListForEach(const TFileList *pFileList, BOOLEAN (*Process)(TFile *pFile, void *pContext),
  BOOLEAN bConsequtiveOrder, void *pContext);
TView *FileListFindFView(const TFileList *pFileList, TFile *pFile);
TBookmarksSet *GetFuncNamesBMSet(const TFileList *pFileList, TFile *pFile);
void DumpFileList(const TFileList *pFileList);

#endif /* ifndef FILENAV_H */

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

