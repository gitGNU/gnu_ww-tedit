/*

File: file2.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 16th November, 1998
Descrition:
  Functions concerning file manipulation in context of platform
  indepentent work space components -- filelist, MRU list, recovery file.

*/

#ifndef FILE2_H
#define FILE2_H

#include "filenav.h"
#include "mru.h"
#include "doctype.h"

void PrepareFileNameTitle(char *sFileName, int nCopy, char *sTitle, int nWidth,
  char *sViewID);

BOOLEAN RemoveTopFile(TFileList *pFileList, TMRUList *pMRUList);

void RemoveLastFile(TFileList *pFileList, TMRUList *pMRUList);

void LoadFile(const char *sFileName, TFileList *pFileList, TMRUList *pMRUList,
  TDocType *pDocTypeSet, BOOLEAN bReload, BOOLEAN bForceReadOnly);

BOOLEAN CheckActivateFile(const char *sFileName, TFileList *pFileList, TMRUList *pMRUList);

void RestoreMRUFiles(TFileList *pFilesList, TMRUList *pMRUList, TDocType *pDocTypeSet);

void LoadMRUFile(int nFile, TFileList *pFileList, TMRUList *pMRUList, TDocType *pDocTypeSet);

TFile *AddNewFile(TFileList *pFileList);

void StoreFile(TFile *pFile, TMRUList *pMRUList);

void StoreFileAs(char *sFileName, TFile *pFile, TFileList *pFileList,
  TMRUList *pMRUList, TDocType *pDocTypeSet);

#endif /* ifndef FILE2_H */

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

