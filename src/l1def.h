/*

File: l1def.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th January, 1999
Descrition:
  Layer1 constants and definitions.

*/

#ifndef L1DEF_H
#define L1DEF_H

#include "doctype.h"

/*
INI file parse errors
*/
#define	ERROR_NO_CLOSING_BRACKET 1  /* Section has no closing bracket */
#define ERROR_INVALID_SECTION_NAME 2  /* Section name must be complent with isalpha() */
#define ERROR_NO_SUCH_SECTION 3  /* Section with particular name not found */
#define ERROR_LINE_EMPTY 4  /* Line is commented or is empty */
#define ERROR_MISSING_EQU_CHAR 5  /* '=' is missing */
#define ERROR_INVALID_NUMBER 6  /* Invalid number specified */
#define ERROR_UNRECOGNIZED_KEY 7  /* Invalid key for the specific section */
#define ERROR_OPCLOSED_EXPECTED 8  /* 'Opened' or 'Closed' in  MRU file */
#define ERROR_INVALID_CURRENT 9  /* 'Current' or '0' expected */
#define ERROR_INVALID_EXT 10  /* File name extention should be in '' */
#define ERROR_EXT_TOO_LONG 11  /* File name extention should be no longer than MAX_EXT_LEN */
#define ERROR_INVALID_FTYPE 12  /* File type should be in '' */
#define ERROR_UNRECOGNIZED_FTYPE 13  /* File type is unknown */
#define ERROR_BAD_TAB 14  /* Bad tab size -- number or range */
#define ERROR_ON_OFF_EXPECT 15  /* 'On' or 'Off' expected */
#define ERROR_INV_RTIME 16  /* Invalid recovery time range */
#define ERROR_INV_RMARG 17  /* Invalid right margin range */
#define ERROR_INV_SAVEMODE 18  /* Invalid file saving mode */
#define ERROR_INVALID_FREADONLY 19  /* Invalid 'ForceReadOnly' in MRU file */
#define ERROR_INVALID_MRUENTRY 20  /* Invalid entry for a MRU item */
#define ERROR_INVALID_DOCENTRY 21  /* Invalid entry for a DOC item */
#define ERROR_FILENAME_TOOLONG_BM 22  /* File name too long in a Bookmark item */
#define ERROR_STRING_TOO_LONG 23  /* String element too long */

/*
String constants
*/
extern const char *sDefaultINIFileName;
extern const char *sDefaultMasterINIFileName;

extern const char *sSection_Version;
extern const char *sSection_FileOpenHistory;
extern const char *sSection_FindHistory;
extern const char *sSection_CalculatorHistory;
extern const char *sSection_FindInFilesHistory;
extern const char *sSection_UserBookmarks;
extern const char *sSection_MRUFiles;
extern const char *sSection_EditorOptions;
extern const char *sSection_DocTypeSet;
extern const char *sSection_DefaultDoc;
extern const char *sSection_ElapsedTime;
extern const char *sSection_Global;
extern const char *sSection_SGeomWin;
extern const char *sSection_SGeomX;

extern const char *sKey_Version;
extern const char *sKey_CfgVersion;
extern const char *sKey_MaxItems;
extern const char *sKey_Line;
extern const char *sKey_Doc;
extern const char *sKEY_MRUFile;
extern const char *sKEY_MRUMax;
extern const char *sKEY_Bookmark;
extern const char *sKey_StrictCheck;
extern const char *sKey_RemoveTrailingBlanks;
extern const char *sKey_Backup;
extern const char *sKey_RestoreLastFiles;
extern const char *sKey_RecoveryTime;
extern const char *sKey_PersistentBlocks;
extern const char *sKey_OverwriteBlocks;
extern const char *sKey_SyntaxHighlighting;
extern const char *sKey_MatchPairHighlighting;
extern const char *sKey_IF0Highlighting;
extern const char *sKey_AscendingSort;
extern const char *sKey_CaseSensitiveSort;
extern const char *sKey_RightMargin;
extern const char *sKey_FileSaveMode;
extern const char *sKey_CombineUndo;
extern const char *sKey_ConsequtiveWinFiles;
extern const char *sKey_StartWithEmptyFile;
extern const char *sKey_Time;

extern const char *sKey_LastTip;
extern const char *sKey_LastBuild;

extern const char *sVal_Closed;
extern const char *sVal_Opened;
extern const char *sVal_Current;
extern const char *sVal_ForceReadOnly;

extern const char *sVal_Auto;
extern const char *sVal_LF;
extern const char *sVal_Unix;
extern const char *sVal_CR;
extern const char *sVal_Mac;
extern const char *sVal_CRLF;
extern const char *sVal_DOS;

extern const char *sKey_Pos;
extern const char *sKey_Size;
extern const char *sKey_Maximized;
extern const char *sKey_Font1;
extern const char *sKey_Font1Sz;
extern const char *sKey_Font1Style;
extern const char *sKey_Font2Style;
extern const char *sKey_Font3Style;

extern const char *sINIFileErrors[];
extern const char *sInfoFileErrors[];

extern const char *sAllMask;
extern const char *sBak;
extern const char *sRec;
extern const char *sRecExt2Mask;
extern const char *sRecExt2;
extern const char *sEndOfFile;
extern const char *sErrnoMsg;
extern const char *sLoading;
extern const char *sLoading2;
extern const char *sExtractingFullName;
extern const char *sNewFileMsg;
extern const char *sNonuniformEOL;
extern const char *sAskSave;
extern const char *sAskSaveName;
extern const char *sSaving;
extern const char *sFileSaved;
extern const char *sConverted;
extern const char *sSaveFailed;
extern const char *sNoMemoryINI;
extern const char *sINIFormatChanged;
extern const char *sINIFormatIsNew;
extern const char *sINIFileNotStored;
extern const char *sMasterINIFileNotStored;
extern const char *sSaveOptions;
extern const char *sNoMemory;
extern const char *sNoMemoryMRU;
extern const char *sNoMemoryLRec;
extern const char *sNoMemoryRec;
extern const char *sRecoverCorrupted1;
extern const char *sRecoverCorrupted2;
extern const char *sRecoverInconsistent;
extern const char *sRecover;
extern const char *sSorted;
extern const char *sIncrementalSearch;
extern const char *sIncrementalSearchBack;
extern const char *sPassedEndOfFile;
extern const char *sAllFilesClosed;
extern const char *sNewFileRec;
extern const char *sEnterLine;
extern const char *sInvalidNumber;
extern const char *sBlanksRemoved;
extern const char *sCanceled;
extern const char *sUnableToOpen;
extern const char *sNoMemoryForFile;
extern const char *sNoMoreMessages;
extern const char *sSearchInProgress;
extern const char *sPassedEndOfTagList;
extern const char *sHelpPageNone;
extern const char *sHelpNoHelpProvided;
extern const char *sHelpNoPointer;
extern const char *sInvalidEsc;
extern const char *sInvalidPatternTerm;
extern const char *sInvalidRefNumber;
extern const char *sInvalidReplPatternTerm;
extern const char *sInvalidPatternOption;
extern const char *sInvalidSwitch;
extern const char *sInvalidSearchModifier;
extern const char *sAskReplace;

#define MAX_BRACES  3
extern char OpenBraces[MAX_BRACES];
extern char CloseBraces[MAX_BRACES];

extern char *sParserTypes[4];
extern char *sEOLTypes[4];
extern char *sEOLTypesDetailed[4];

extern TDocType DefaultDocuments[];
extern TDocType _DefaultDocOptions;


extern const char *sNoname;

#endif  /* ifndef L1DEF_H */

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

