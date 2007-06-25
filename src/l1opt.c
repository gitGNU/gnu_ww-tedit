/*

File: l1opt.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Layer1 options and control variables.

*/

#include "global.h"
#include "l1opt.h"

BOOLEAN bStrictCheck = FALSE;  /* Whether to check strictly the key sequences */
BOOLEAN bRemoveTrailingBlanks = TRUE;
int nTabSize = 8;
BOOLEAN bTabLeft;  /* Internal */
BOOLEAN bInsert = TRUE;
BOOLEAN bPersistentBlocks = FALSE;
BOOLEAN bIncrementalSearch = FALSE;
BOOLEAN bPreserveIncrementalSearch = FALSE;  /* Last cmd was search oriented */
BOOLEAN bBlockMarkMode = FALSE;
BOOLEAN bRestoreLastFiles = FALSE;  /* Reload the files from the last session */
BOOLEAN bBackup = TRUE;
BOOLEAN bOverwriteBlocks = TRUE;
BOOLEAN bSyntaxHighlighting = TRUE;
BOOLEAN bMatchPairHighlighting = TRUE;
BOOLEAN bIF0Highlighting = TRUE;
BOOLEAN bAscendingSort = TRUE;
BOOLEAN bCaseSensitiveSort = TRUE;
int nRightMargin = 70;
BOOLEAN bCombineUndo = TRUE;
int nFileType = 0;  /* Values defined in doctype.h */
BOOLEAN bUseTabs = TRUE;
BOOLEAN bOptimalFill = TRUE;
BOOLEAN bAutoIndent = TRUE;
BOOLEAN bBackspaceUnindent = TRUE;
BOOLEAN bCursorThroughTabs = FALSE;
BOOLEAN bWordWrap = FALSE;
int nFileSaveMode = -1;  /* now is Auto, otherwise EOLTypes should be used */
BOOLEAN bStartWithEmptyFile = TRUE;  /* This option is ignored! */

TDocType DocumentTypes[MAX_DOCS];
TDocType DefaultDocOptions;

int nLastTip = 0;
int nLastBuildAnounced = 0;

int nPreISearch_Col;  /* Preserve the positions prior  */
int nPreISearch_Row;  /* invoking CmdIncrementalSearch */

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

