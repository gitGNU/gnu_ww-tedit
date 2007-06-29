/*

File: l1opt.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Layer1 options, definitions and control variables.

*/

#ifndef	L1OPT_H
#define L1OPT_H

#include "wlimits.h"
#include "doctype.h"

#define FILE_NAME_FMT "(filename)"	/* (filename) in format string */

/*
Message box flags
*/
#define MSG_OK	0x1	/* Display a message and wait for a key (OK button) */
#define MSG_YESNO  0x2	/* Display a message + Yes + No */
#define MSG_YESNOCANCEL  0x4  /* Display a message + Yes + No +	Esc (Cancel) */
#define MSG_STATONLY  0x8  /* This is only a status line message (no key) */
#define MSG_WARNING  0x10  /* This is a warning message */
#define MSG_ERROR  0x20  /* This is an error message */
#define MSG_INFO  0x40  /* This is an information message */
#define MSG_ERRNO  0x80  /* Display errno message */

/*
Options
*/
extern BOOLEAN bStrictCheck;  /* Whether to check strictly the key sequences */
extern BOOLEAN bRemoveTrailingBlanks;
extern int nTabSize;
extern BOOLEAN bTabLeft;
extern BOOLEAN bInsert;
extern BOOLEAN bCursorThroughTabs;
extern BOOLEAN bPersistentBlocks;
extern BOOLEAN bIncrementalSearch;
extern BOOLEAN bBlockMarkMode;
extern BOOLEAN bPreserveIncrementalSearch;  /* Last cmd was search oriented */
extern BOOLEAN bRestoreLastFiles;  /* Reload the files from the last session */
extern BOOLEAN bBackup;
extern BOOLEAN bOverwriteBlocks;
extern BOOLEAN bSyntaxHighlighting;
extern BOOLEAN bMatchPairHighlighting;
extern BOOLEAN bIF0Highlighting;
extern BOOLEAN bAscendingSort;
extern BOOLEAN bCaseSensitiveSort;
extern int nRightMargin;
extern BOOLEAN bCombineUndo;
extern int nFileType;  /* Values defined in doctype.h */
extern BOOLEAN bUseTabs;
extern BOOLEAN bOptimalFill;
extern BOOLEAN bAutoIndent;
extern BOOLEAN bBackspaceUnindent;
extern BOOLEAN bCursorThroughTabs;
extern BOOLEAN bWordWrap;
extern int nFileSaveMode;
extern BOOLEAN bStartWithEmptyFile;

extern int nLastTip_Opt;
extern int nLastBuildAnounced_Opt;
extern TDocType DocumentTypes[MAX_DOCS];
extern TDocType DefaultDocOptions;

extern int nPreISearch_Col;  /* Preserve the positions prior  */
extern int nPreISearch_Row;  /* invoking CmdIncrementalSearch */

#endif /* #ifndef L1OPT_H */

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

