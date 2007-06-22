/*

File: file.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Definitions and structures concerning file storage and manipulation.

*/

#ifndef FILE_H
#define FILE_H

#include "tarray.h"
#include "clist.h"
#include "maxpath.h"
#include "path.h"
#include "l1opt.h"

/* TLine.attr bit mask. empty part is an actual syntax status value */
#define SYNTAX_STATUS_SET  0x80000000
#define LINE_SYNTAX_STATUS(s) ((DWORD)(s) & ~SYNTAX_STATUS_SET)
typedef struct Line
{
  char *pLine;  /* Points to an ASCIIZ string */
  char *pFileBlock;  /* The line is somwhere inside this block */
  int nLen;
  DWORD attr;
} TLine;

/* TFile.blockattr bit mask definitions */
#define COLUMN_BLOCK	1

enum EOLTypes
{
  CRLFtype = 0,
  LFtype,
  CRtype
};  /* do not reorder without changing sEOLTypes[], Auto is -1 */

typedef struct block
{
  #ifdef _DEBUG
  BYTE MagicByte;
  #define BLOCK_MAGIC  0x52
  #endif
  TArray(TLine) pIndex;  /* TLine *pIndex */
  char *pBlock; /* The whole of the text block resides in a single memory block */
  WORD blockattr;  /* Column or character */
  int nNumberOfLines;  /* How much lines comprises the block */
  int nEOLType;
} TBlock;

#ifdef _DEBUG
#define VALID_PBLOCK(pBlock) ((pBlock) != NULL && (pBlock)->MagicByte == BLOCK_MAGIC)
#else
#define VALID_PBLOCK(pBlock) (1)
#endif

/* Action codes */
#define	acINSERT 1  /* Block of text is inserted */
#define acDELETE 2  /* Block of text is	deleted */
#define acREPLACE 3  /* Text from the current line is replaced (OVR mode) */
#define acREARRANGE 4  /* Most likely as result from CmdEditSort() */
#define acREARRANGEBACK 5  /* Can appear only from a recovery file */

typedef struct FileStatus
{
  int nRow;  /* Position of the cursor: Row */
  int nCol;  /* Position of the cursor: Col */

  /* Page position */
  int nTopLine;  /* The number of the top line of current page */
  int nWrtEdge;  /* The position of the most left character visible on the scr */

  int nNumberOfLines;  /* The number of the lines in the file */

  /* Block parameters */
  int nStartLine;  /* Line number where block stared */
  int nEndLine;  /* Line number where block ended */
  int nStartPos;  /* For column block this is the column pos of the left edge */
  int nEndPos;  /* For column block this is the column pos of the right edge */
  WORD blockattr;
  BOOLEAN bBlock;

  BOOLEAN bChanged;  /* File in memory has changed from its disk image */
} TFileStatus;

typedef struct UndoRecord
{
  #ifdef _DEBUG
  BYTE MagicByte;
  #define UNDOREC_MAGIC 0x53
  #endif

  int nUndoBlockID;  /* Consequtive number */

  int nUndoLevel;  /* This is operations nesting level; End of atom indicator */
  BOOLEAN bUndone;  /* This block was undone by CmdEditUndo() */
  BOOLEAN bRecoveryStore;  /* This block was stored as item in recovery	file */

  TFileStatus before;  /* File status before performing the action */
  TFileStatus after;  /* File status after performing the action */

  /* Data as a result of user action */
  void *pData;
  int nStart;  /* pData coordinates in the file */
  int nEnd;
  int nStartPos;
  int nEndPos;
  WORD blockattr;  /* Used only when loaded from a recovery file */

  int nOperation;
} TUndoRecord;

/* To be used in ASSERT()! */
#ifdef _DEBUG
#define VALID_PUNDOREC(pUndoRec) (pUndoRec != NULL && pUndoRec->MagicByte == UNDOREC_MAGIC)
#else
#define VALID_PUNDOREC(pUndoRec) (1)
#endif

/*
NOTE:
Adding fields here should be folowed by a correction of InitEmptyFile().
*/
typedef struct txtf
{
  #ifdef _DEBUG
  BYTE MagicByte;
  #define FILE_MAGIC 0x50
  #endif

  TArray(TLine) pIndex;  /* TLine *pIndex */
  TListRoot blist;  /* The file is a sequence of blocks containing lines */

  int nEOLType;  /* CR/LF, CR, LF */
  /* The 3 fields below are only to be show in the diagnostic terminal */
  int nCR;  /* To be filled by LoadFile() */
  int nLF;  /* To be filled by LoadFile() */
  int nCRLF;  /* To be filled by LoadFile() */
  int nZero;  /* To be filled by LoadFile() */
  int nEOLTypeDisk;  /* Last format stored on disk */

  char sFileName[_MAX_PATH];  /* File name and full path */
  char sTitle[_MAX_PATH];  /* What to show as file title */
  int nCopy;  /* If there are more that one copy loaded */
  int nType;  /* Syntax type of this file */

  char sMsg[MAX_FILE_MSG_LEN];  /* Message to be displayed at status line */

  TTime LastWriteTime;  /* disk time */
  int nFileSize;  /* File size as stored at the disk in bytes */

  /* Cursor position */
  int nRow;  /* Position of the cursor: Row */
  int nCol;  /* Position of the cursor: Col */
  char *pCurPos;  /* Position into the current line */
  int x;  /* File window relevent x cursor position */
  int y;  /* File window relevent y cursor position */
  DWORD lnattr;  /* Before insert of delete operations */
  BOOLEAN bShowBlockCursor;

  /* Page position */
  int nTopLine;  /* The number of the top line of current page */
  int nWrtEdge;  /* The position of the most left character visible on the scr */

  int nNumVisibleLines;  /* Specified by TFileView.HandleEvent() */

  int nNumberOfLines;  /* The number of the lines in the file */

  /* Block parameters */
  BOOLEAN bBlock;
  int nStartLine;
  int nEndLine;
  int nStartPos;  /* For column block this is the column pos of the left edge */
  int nEndPos;  /* For column block this is the column pos of the right edge */
  WORD blockattr;

  /* Marking block transition parameters */
  int nExpandCol;
  int nExpandRow;
  int nLastExtendCol;
  int nLastExtendRow;
  int nExtendAncorCol;
  int nExtendAncorRow;
  BOOLEAN bPreserveSelection;  /* Preserve the selection in non-persistent mode */

  /* Tracks record of previous cursor position */
  int nPrevCol;
  int nPrevRow;

  /* Undo records */
  TArray(TUndoRecord) pUndoIndex;
  int nUndoLevel;
  int nNumberOfRecords;
  int nUndoIDCounter;

  BOOLEAN bChanged;  /* File in memory has changed from its disk image */
  BOOLEAN bRecoveryStored;
  BOOLEAN bFileNameChanged;  /* Set by CmdFileSaveAs() */
  BOOLEAN bForceNewRecoveryFile;  /* Set by CmdFileSave() */
  BOOLEAN bForceReadOnly;  /* If no editing allowed (CmdFileOpenAsReadOnly) */
  BOOLEAN bReadOnly;  /* Copy of the file attribute */
  BOOLEAN bNew;  /* File is created and	as a new file in memory */

  /* Some flags to limit what to be displayed on the status line */
  BOOLEAN bDisplayChanged;
  BOOLEAN bDisplayInsertMode;
  BOOLEAN bDisplayColumnMode;
  BOOLEAN bDisplayReadOnly;

  /*
  Recovery file fields;
  Recovery file status line (if recovery file discovered during file loading);
  */
  char sRecoveryFileName[_MAX_PATH];  /* Recovery file name and full path */
  int nRecStatFileSize;
  int nRecStatMonth;
  int nRecStatDay;
  int nRecStatYear;
  int nRecStatHour;
  int nRecStatMin;
  int nRecStatSec;

  int nID;  /* for each file a unique ID is maintained */

  /*
  Update screen/line request flags
  */
  BOOLEAN bUpdatePage;
  BOOLEAN bUpdateLine;
  BOOLEAN bUpdateStatus;

  /*
  Option: Different message for end-of-file line
  */
  char *sEndOfFile;

  /*
  Reverse mapping to some fields in TFileListItem and FileView
  */
  void *pBMSetFuncNames;
  void *pBMFuncFindCtx;

  /*
  Matching brackets highlight regions
  */
  BOOLEAN bHighlighAreas;
  struct { int c, r, l; } hlightareas[6];
  char *sTooltipBuf;
  int nTooltipBufSize;
  int bTooltipIsTop;
  int nNumTooltipLines;
} TFile;

/* To be used in ASSERT()! */
#ifdef _DEBUG
#define VALID_PFILE(pFile) ((pFile) != NULL && (pFile)->MagicByte == FILE_MAGIC)
#else
#define VALID_PFILE(pFile) (1)
#endif

#define FILE_IS_EMPTY(pFile)  ((pFile)->nNumberOfLines == 0)
#define INDEX_IN_LINE(pFile, nLine, pPos)  (pPos - GetLineText(pFile, nLine))
#define IS_VALID_CUR_POS(pFile) (INDEX_IN_LINE(pFile, (pFile)->nRow, (pFile)->pCurPos) \
  <= GetLine(pFile, (pFile)->nRow)->nLen)

void InitEmptyFile(TFile *pFile);
int LoadFilePrim(TFile *pFile);
void DisposeFile(TFile *pFile);

TLine *GetLine(const TFile *pFile, int nLine);
#define GetLineText(pFile, nLine)  (GetLine(pFile, nLine)->pLine)
BOOLEAN LineIsInBlock(const TFile *pFile, int nLine);

int StoreFilePrim(const TFile *pFile, int nOutputEOLType);

struct txtf_edit_flags
{
  int syntax_highlighting;
  int match_pair_highlighting;
  int tab_size;
};

#endif  /* ifndef FILE_H */

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

