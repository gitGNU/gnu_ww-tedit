/*

File: blockcmd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 7th April, 1999
Descrition:
  Commands concerning block manipulation.

*/

#ifndef BLOCKCMD_H
#define BLOCKCMD_H

void CmdEditMarkBlockBegin(void *pCtx);
void CmdEditMarkBlockEnd(void *pCtx);
void CmdEditToggleBlockHide(void *pCtx);
void CmdEditToggleBlockMarkMode(void *pCtx);
void CmdEditSelectAll(void *pCtx);
void CmdEditCharLeftExtend(void *pCtx);
void CmdEditCharRightExtend(void *pCtx);
void CmdEditLineUpExtend(void *pCtx);
void CmdEditLineDownExtend(void *pCtx);
void CmdEditPageUpExtend(void *pCtx);
void CmdEditPageDownExtend(void *pCtx);
void CmdEditHomeExtend(void *pCtx);
void CmdEditEndExtend(void *pCtx);
void CmdEditTopFileExtend(void *pCtx);
void CmdEditBottomFileExtend(void *pCtx);
void CmdEditNextWordExtend(void *pCtx);
void CmdEditPrevWordExtend(void *pCtx);

void CmdEditDeleteBlock(void *pCtx);
void CmdEditDel(void *pCtx);
void CmdEditCut(void *pCtx);
void CmdEditCopy(void *pCtx);
void CmdEditPaste(void *pCtx);
void CmdEditClipboardHistory(void *pCtx);
void CmdEditSort(void *pCtx);
void CmdEditTabify(void *pCtx);
void CmdEditUntabify(void *pCtx);
void CmdEditUppercase(void *pCtx);
void CmdEditLowercase(void *pCtx);
void CmdEditIndent(void *pCtx);
void CmdEditTrimTrailingBlanks(void *pCtx);
void CmdEditUnindent(void *pCtx);
void CmdEditTab(void *pCtx);
void CmdEditTabBack(void *pCtx);

void AllocKeyBlock(void);
void DisposeKeyBlock(void);
void ExamineKey(TFile *pFile, DWORD dwKey, BOOLEAN bAutoIncrementalMode);

#endif  /* ifndef BLOCKCMD_H */

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

