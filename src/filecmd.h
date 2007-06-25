/*

File: filecmd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 22nd December, 1998
Descrition:
  File commands.

*/

#ifndef FILECMD_H
#define FILECMD_H

#include "file.h"
#include "block.h"
#include "mru.h"
#include "search.h"
#include "contain.h"

BOOLEAN CheckToRemoveBlankFile(void);
void RemoveBlankFile(BOOLEAN bRemoveFlag);

void CmdFileNew(void *pCtx);
void CmdFileOpen(void *pCtx);
void CmdFileOpenAsReadOnly(void *pCtx);
void CmdFileSave(void *pCtx);
void CmdFileSaveAs(void *pCtx);
void CmdFileSaveAll(void *pCtx);
void CmdFileClose(void *pCtx);
void CmdFileExit(void *pCtx);
void CmdFileOpenMRU1(void *pCtx);
void CmdFileOpenMRU2(void *pCtx);
void CmdFileOpenMRU3(void *pCtx);
void CmdFileOpenMRU4(void *pCtx);
void CmdFileOpenMRU5(void *pCtx);
void CmdFileOpenMRU6(void *pCtx);
void CmdFileOpenMRU7(void *pCtx);
void CmdFileOpenMRU8(void *pCtx);
void CmdFileOpenMRU9(void *pCtx);
void CmdFileOpenMRU10(void *pCtx);

void ProcessCommandLine(int argc, char **argv);

void DumpFile(const TFile *pFile);
void DumpBlock(const TBlock *pBlock, int nHeaderLines);
void DumpMRUList(TMRUList *pMRUList);
void DumpUndoIndex(TFile *pFile);
void DumpFuncList(TFile *pFile);
void DumpBookmarks(void);
void DumpSearchReplacePattern(TSearchContext *pstSearchContext);
void DumpContainersTree(TContainer *pCont, int indent);

void CloseSmallTerminal(void);
void OpenSmallTerminal(void);
void PrintString(const char *fmt, ...);

void CmdHelpAbout(void *pCtx);

#endif	/* ifndef FILECMD_H */

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

