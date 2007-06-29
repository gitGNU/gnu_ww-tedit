/*

File: options.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Options, dinitions and control variables for Layer2.

*/

#ifndef	OPTIONS_H
#define OPTIONS_H

#include "doctype.h"

#define MAX_MRUITEM_LEN  50  /* Width in Root|File submenu */
#define MAX_WINITEM_LEN  35  /* Width in Root|Width submenu */

extern int nRecoveryTime;
extern BOOLEAN bConsequtiveWinFiles;  /* How to organize 'Window' submenu */

extern BOOLEAN bStrictCheck_Opt;  /* available only by editing .ini file */
extern BOOLEAN bRemoveTrailingBlanks_Opt;
extern BOOLEAN bPersistentBlocks_Opt;
extern BOOLEAN bRestoreLastFiles_Opt;
extern BOOLEAN bBackup_Opt;
extern BOOLEAN bOverwriteBlocks_Opt;
extern BOOLEAN bSyntaxHighlighting_Opt;
extern BOOLEAN bMatchPairHighlighting_Opt;
extern BOOLEAN bIF0Highlighting_Opt;
extern BOOLEAN bAscendingSort_Opt;
extern BOOLEAN bCaseSensitiveSort_Opt;
extern int nRightMargin_Opt;
extern BOOLEAN bCombineUndo_Opt;
extern int nRecoveryTime_Opt;
extern BOOLEAN bConsequtiveWinFiles_Opt;  /* available only by editing .ini file */
extern int nFileSaveMode_Opt;
extern BOOLEAN bStartWithEmptyFile_Opt;
TDocType DocumentTypes_Opt[MAX_DOCS];
extern TDocType DefaultDocOptions_Opt;

extern int nLastTip;
extern int nLastBuildAnounced;

void CopyToEditorOptions(void);
void CopyFromEditorOptions(void);
void CmdOptionsSave(void *pCtx);
void CmdOptionsToggleInsertMode(void *pCtx);
void CmdOptionsToggleColumnBlock(void *pCtx);

#endif  /* ifndef OPTIONS_H */

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

