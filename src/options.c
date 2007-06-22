/*

File: options.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Options and control variables for Layer2.

*/

#include "global.h"
#include "l1opt.h"
#include "l1def.h"
#include "doctype.h"
#include "l2disp.h"
#include "ini.h"
#include "ini2.h"
#include "defs.h"
#include "wrkspace.h"
#include "main2.h"
#include "disp.h"
#include "enterln.h"
#include "cmdc.h"
#include "options.h"

int nRecoveryTime = 15;  /* seconds */
BOOLEAN bConsequtiveWinFiles = FALSE;  /* How to organize 'Window' submenu */

/*
Exact copy of the options as read from the .ini file

Q: Why is necessary to be maintained such a copy of the options?
A: Lets take for example bPersistentBlocks options: When user changes
PersistentBlock On/Off from the options menu it changes
bPersistentBlocks flag. To make the new value permanent, should be
executed Options|Save_options. If options were not explicitely stored
PersistentBlock should remain the same opening next session of the
editor. That is the purpose of bPersistentBlocks_Opt variable -- to
store the original value as read from the .ini file, and to be stored
upon exiting the editor. bPersistentBlock_Opt will be equal to
bPersistentBlock after Store_option operation is performed.
*/
BOOLEAN bStrictCheck_Opt;  /* available only by editing .ini file */
BOOLEAN bRemoveTrailingBlanks_Opt;
BOOLEAN bPersistentBlocks_Opt;
BOOLEAN bRestoreLastFiles_Opt;
BOOLEAN bBackup_Opt;
BOOLEAN bOverwriteBlocks_Opt;
BOOLEAN bSyntaxHighlighting_Opt;
BOOLEAN bMatchPairHighlighting_Opt;
BOOLEAN bIF0Highlighting_Opt;
BOOLEAN bAscendingSort_Opt;
BOOLEAN bCaseSensitiveSort_Opt;
int nRightMargin_Opt;
BOOLEAN bCombineUndo_Opt;
int nRecoveryTime_Opt;
BOOLEAN bConsequtiveWinFiles_Opt;  /* available only by editing .ini file */
int nFileSaveMode_Opt;
BOOLEAN bStartWithEmptyFile_Opt;

int nLastTip_Opt;
int nLastBuildAnounced_Opt;
TDocType DocumentTypes_Opt[MAX_DOCS];
TDocType DefaultDocOptions_Opt;

/* ************************************************************************
   Function: CopyToEditorOptions
   Description:
     Copies from editor options to their XXX_Opt counterparts.
*/
void CopyToEditorOptions(void)
{
  bStrictCheck = bStrictCheck_Opt;
  bRemoveTrailingBlanks = bRemoveTrailingBlanks_Opt;
  bPersistentBlocks = bPersistentBlocks_Opt;
  bRestoreLastFiles = bRestoreLastFiles_Opt;
  bBackup = bBackup_Opt;
  bOverwriteBlocks = bOverwriteBlocks_Opt;
  bSyntaxHighlighting = bSyntaxHighlighting_Opt;
  bMatchPairHighlighting = bMatchPairHighlighting_Opt;
  bIF0Highlighting = bIF0Highlighting_Opt;
  bAscendingSort = bAscendingSort_Opt;
  bCaseSensitiveSort = bCaseSensitiveSort_Opt;
  nRightMargin = nRightMargin_Opt;
  bCombineUndo = bCombineUndo_Opt;
  nRecoveryTime = nRecoveryTime_Opt;
  bConsequtiveWinFiles = bConsequtiveWinFiles_Opt;
  nFileSaveMode = nFileSaveMode_Opt;
  bStartWithEmptyFile = bStartWithEmptyFile_Opt;
}

/* ************************************************************************
   Function: CopyFromEditorOptions
   Description:
     Copies from XXX_Opt option set to their working copy counterparts.
*/
void CopyFromEditorOptions(void)
{
  bStrictCheck_Opt = bStrictCheck;
  bRemoveTrailingBlanks_Opt = bRemoveTrailingBlanks;
  bPersistentBlocks_Opt = bPersistentBlocks;
  bRestoreLastFiles_Opt = bRestoreLastFiles;
  bBackup_Opt = bBackup;
  bOverwriteBlocks_Opt = bOverwriteBlocks;
  bSyntaxHighlighting_Opt = bSyntaxHighlighting;
  bMatchPairHighlighting_Opt = bMatchPairHighlighting;
  bIF0Highlighting_Opt = bIF0Highlighting;
  bAscendingSort_Opt = bAscendingSort;
  bCaseSensitiveSort_Opt = bCaseSensitiveSort;
  nRightMargin_Opt = nRightMargin;
  bCombineUndo_Opt = bCombineUndo;
  nRecoveryTime_Opt = nRecoveryTime;
  bConsequtiveWinFiles_Opt = bConsequtiveWinFiles;
  nFileSaveMode_Opt = nFileSaveMode;
  bStartWithEmptyFile_Opt = bStartWithEmptyFile;
}

/* ************************************************************************
   Function: CmdOptionsSave
   Description:
*/
void CmdOptionsSave(void *pCtx)
{
  char sININame[_MAX_PATH];
  disp_char_buf_t *pStatLn;
  char sOldINIFileName[_MAX_PATH];
  disp_elapsed_time_t t;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  pStatLn = SaveStatusLine(disp);

  FSplit(sINIFileName, sINIFilePath, sININame, sAllMask, TRUE, TRUE);

  /* MAX_EXT_LEN - 1 to put '.' in case user missed to enter it */
  if (EnterLn(sSaveOptionsPrompt, sINIFilePath, _MAX_PATH, NULL, NULL, NULL, NULL, TRUE, disp))
  {
    if (sINIFilePath[0] == '\0')
      GetCurDir(sINIFilePath);

    strcpy(sOldINIFileName, sINIFileName);

    strcpy(sINIFileName, sINIFilePath);
    AddTrailingSlash(sINIFileName);
    strcat(sINIFileName, sININame);

    GetFullPath(sINIFileName);

    if (filestrcmp(sOldINIFileName, sINIFileName) != 0)
    {
      /* options file cloned: reset the elapsed time */
      t.hours = 0;
      t.minutes = 0;
      t.seconds = 0;
      disp_elapsed_time_set(disp, &t);
    }

    /* Now store what was changed in the working copyes */
    CopyDocumentTypeSet(DocumentTypes_Opt, DocumentTypes);
    memcpy(&DefaultDocOptions_Opt, &DefaultDocOptions, sizeof(TDocType));
    CopyFromEditorOptions();

    StoreMasterOptions(TRUE, disp);
    StoreWorkspace(disp);
  }

  RestoreStatusLine(pStatLn, disp);
}

/* ************************************************************************
   Function: CmdOptionsToggleInsertMode
   Description:
*/
void CmdOptionsToggleInsertMode(void *pCtx)
{
  bInsert = !bInsert;
}

/* ************************************************************************
   Function: CmdOptionsToggleColumnBlock
   Description:
*/
void CmdOptionsToggleColumnBlock(void *pCtx)
{
  TFile *pFile;

  pFile = CMDC_PFILE(pCtx);
  pFile->blockattr ^= COLUMN_BLOCK;
  pFile->bBlock = FALSE;

  pFile->bUpdateStatus = TRUE;
}

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

