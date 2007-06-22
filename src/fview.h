/*

File: fview.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 11st July, 2002
Descrition:
  Implementation of a file view.

*/

#ifndef _FVIEW_H
#define _FVIEW_H

#include "wline.h"
#include "file.h"
#include "contain.h"
#include "cmd.h"

typedef struct _FileView
{
  TFile *pFile;
  void *pReserved;  /* Extra data for the file view, NULL by default */
  TKeySequence KeySeq;
  BOOLEAN (*pfnExecuteCommand)(const TCmdDesc *pCommands,
    int nCode, int nNumberOfElements, void *pCtx);
  void (*pfnExamineKey)(void *pCtx, DWORD dwKey, BOOLEAN bAutoIncrementalMode);
  void (*pfnUpdateStatusLine)(TFile *pCtx);
  TKeySequence *pKeySequences;
  TCmdDesc *pCommands;
  int nNumberOfCommands;
  int nPaletteStart;
  BOOLEAN bColorInterfActivated;
  TExtraColorInterf ExtraColorInterf;
  BYTE FuncNameCtx[88];  /* to store TBMFindNextCtx */
  BOOLEAN bAutoIncrementalSearch;
} TFileView;

void ExecuteFileCommand(int nCmdCode);
void FileViewInit(TView *pView, TFile *pFile, TFileView *pFileView);
TContainer *FileViewFindContainer(TFileView *pFileView);

#endif  /* #ifndef _FVIEW_H */

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

