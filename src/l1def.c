/*

File: l1def.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th January, 1999
Descrition:
  Layer1 constants and definitions.

*/

#include "global.h"
#include "doctype.h"
#include "l1def.h"

const char *sDefaultINIFileName = "w.ini";
const char *sDefaultMasterINIFileName = "wglob.ini";
const char *sINIFileErrors[] =
{
  "No closing bracket",
  "Invalid section name",
  "Section not found",
  "Line ie empty",
  "Missing '='",
  "Invalid number",
  "Unrecognized key",
  "'Opened' or 'Closed' expected",
  "Invalid value for 'Current'",
  "Invalid string for 'Extention'",
  "'Extention' is too long",
  "Bad string for 'Doc type'",
  "Unknown 'Doc type'",
  "Bad tab size",
  "On/Off expected",
  "Invalid range for Recovery time",
  "Invalid range for Right margin",
  "Invalid file saving mode",
  "Invalid value for 'ForceReadOnly'",
  "Invalid entry for MRU item",
  "Invalid entry for DOC item",
  "File name too long in a Bookmark item",
  "String element too long"
};

const char *sInfoFileErrors[] =
{
  "Failed to open help file",
  "Indirect file list not found",
  "Indirect file name too long",
  "Unexpected end of data or internal error",
  "Invalid indirect file list format",
  "Indirect file index number too long",
  "Indirect file name list indicated no memory to add entry",
  "Indirect file name list contains has invalid index value",
  "Node name too long",
  "Node offset number too long",
  "Node invalid offset value",
  "No memory to add node entry",
  "Unable to open info page file",
  "Info page file read error",
  "No memory to load info page data",
  "Help entry not found"
};

const char *sSection_Version = "Version";
const char *sSection_FileOpenHistory = "FileOpenHistory";
const char *sSection_FindHistory = "FindHistory";
const char *sSection_CalculatorHistory = "CalculatorHistory";
const char *sSection_FindInFilesHistory = "FindInFilesHistory";
const char *sSection_UserBookmarks = "UserBookmarks";
const char *sSection_MRUFiles = "MRUFiles";
const char *sSection_EditorOptions = "EditorOptions";
const char *sSection_DocTypeSet = "DocumentTypeSet";
const char *sSection_DefaultDoc = "DefaultDocument";
const char *sSection_ElapsedTime = "ElapsedTime";
const char *sSection_Global = "Global";
const char *sSection_SGeomWin = "GeometryWin";
const char *sSection_SGeomX = "GeometryX";

const char *sKey_Version = "Version";
const char *sKey_CfgVersion = "CfgVersion";
const char *sKey_MaxItems = "MaxItems";
const char *sKey_Line = "Line";
const char *sKey_Doc = "Doc";
const char *sKEY_MRUFile = "File";
const char *sKEY_MRUMax = "MaxMRUFiles";
const char *sKEY_Bookmark = "Bookmark";
const char *sKey_StrictCheck = "StrictCheck";
const char *sKey_RemoveTrailingBlanks = "RemoveTrailingBlanks";
const char *sKey_Backup	= "Backup";
const char *sKey_RestoreLastFiles = "RestoreLastFiles";
const char *sKey_RecoveryTime =	"RecoveryTime";
const char *sKey_PersistentBlocks = "PersistentBlocks";
const char *sKey_OverwriteBlocks = "OverwriteBlocks";
const char *sKey_SyntaxHighlighting = "SyntaxHighlighting";
const char *sKey_MatchPairHighlighting = "MatchPairHighlighting";
const char *sKey_IF0Highlighting = "IF0Highlighting";
const char *sKey_AscendingSort = "AscendingSort";
const char *sKey_CaseSensitiveSort = "CaseSensitiveSort";
const char *sKey_RightMargin = "RightMargin";
const char *sKey_FileSaveMode = "FileSaveMode";
const char *sKey_CombineUndo = "CombineUndo";
const char *sKey_ConsequtiveWinFiles = "ConsequtiveWinFiles";
const char *sKey_StartWithEmptyFile = "StartWithEmptyFile";
const char *sKey_Time = "Time";

const char *sKey_LastTip = "LastTip";
const char *sKey_LastBuild = "LastBuild";

const char *sVal_Closed = "Closed";
const char *sVal_Opened = "Opened";
const char *sVal_Current = "Current";
const char *sVal_ForceReadOnly = "ForceReadOnly";

const char *sVal_Auto = "Auto";
const char *sVal_LF = "LF";
const char *sVal_Unix = "Unix";
const char *sVal_CR = "CR";
const char *sVal_Mac = "Mac";
const char *sVal_CRLF = "CRLF";
const char *sVal_DOS = "DOS";

const char *sKey_Pos = "Pos";
const char *sKey_Size = "Size";
const char *sKey_Maximized = "Maximized";
const char *sKey_Font1 = "Font1";
const char *sKey_Font1Sz = "Font1Sz";
const char *sKey_Font1Style = "Font1Style";
const char *sKey_Font2Style = "Font2Style";
const char *sKey_Font3Style = "Font3Style";

#ifdef MSDOS
const char *sAllMask = "*.*";
#else
const char *sAllMask = "*";
#endif
const char *sBak = ".bak";
const char *sRec = ".rec";
const char *sRecExt2Mask = ".r*";
const char *sRecExt2 = ".r%02d";
const char *sEndOfFile = "<*** End Of File ***>";
const char *sErrnoMsg = "file (filename): ";
const char *sLoading = "Loading...";
const char *sLoading2 = "Loading (filename)....";
const char *sExtractingFullName = "Extracting full name...";
const char *sNewFileMsg = "New file";
const char *sNonuniformEOL = "Warning: Nonuniform end-of-line markers detected";
const char *sAskSave = "Save";
const char *sAskSaveName = "Save (filename)";
const char *sSaving = "Saving (filename)...";
const char *sFileSaved = "%s saved";
const char *sConverted = " (converted to %s text format)";
const char *sSaveFailed = "Failed to save (filename)";
const char *sNoMemoryINI = "No enough memory to process the INI file";
const char *sINIFormatChanged = "New cfg format - partial conversion supplied";
const char *sINIFormatIsNew = "The cfg file is of a new unsupported version type.";
const char *sINIFileNotStored = "Configuration not stored. INI file unchanged.";
const char *sMasterINIFileNotStored = "Configuration not stored. Master INI file unchanged.";
const char *sSaveOptions = "Storing (filename) configuration file...";
const char *sNoMemory = "No enough memory for this operation";
const char *sNoMemoryMRU = "No enough memory to load all the most recently used files";
const char *sNoMemoryLRec = "No memory to load the recovery file";
const char *sNoMemoryRec = "No memory to apply the recovery file";
const char *sRecoverCorrupted1 = "Recovery file (filename) bad. Partial recovery.";
const char *sRecoverCorrupted2 = "Recovery file (filename) bad. Recovery impossible.";
const char *sRecoverInconsistent = "Recovery file (filename) bad. Inconsistency found.";
const char *sRecover = "File (filename) was not saved. Recover";
const char *sIncrementalSearch = "Incremental search:";
const char *sIncrementalSearchBack = "Incremental search (back):";
const char *sPassedEndOfFile = "<Passed end of file line>";
const char *sAllFilesClosed = "All files have been closed";
const char *sNewFileRec = "newfile";
const char *sEnterLine = "Line:";
const char *sInvalidNumber = "Invalid number \"%s\"";
const char *sBlanksRemoved = "%d blank characters removed";
const char *sSorted = "%d lines sorted";
const char *sCanceled = "Canceled";
const char *sUnableToOpen = "Unable to perform read operation on ";
const char *sNoMemoryForFile = "No memory to load ";
const char *sNoMoreMessages = "---no more messages---";
const char *sSearchInProgress = "Searching...";
const char *sPassedEndOfTagList = "<Passed end of the tag list>";
const char *sHelpPageNone = "NONE";
const char *sHelpNoHelpProvided = "No help provided for this item";
const char *sHelpNoPointer = "No help page pointer for this item";
const char *sInvalidEsc = "Invalid escape of character at the end of the pattern";
const char *sInvalidPatternTerm = "Invalid termination of search pattern";
const char *sInvalidRefNumber = "Invalid reference number";
const char *sInvalidReplPatternTerm = "Invalid termination of replace pattern";
const char *sInvalidPatternOption = "Invalid pattern option";
const char *sInvalidSwitch = "Invalid switch '%s' of '%s'";
const char *sInvalidSearchModifier = "Invalid option '%s'";
const char *sAskReplace = "Replace? (Y/N/ESC/All)";

char OpenBraces[MAX_BRACES] = {'(', '{', '['};
char CloseBraces[MAX_BRACES] = {')', '}', ']'};

char *sParserTypes[4] =
{
  "C/C++",      /* type 0 */
  "80x86-ASM",  /* type 1 */
  "8051-A51",   /* type 2 */
  "plain",      /* type 3 */
};

char *sEOLTypes[4] =
{
  "Auto",
  "DOS",
  "Unix",
  "Mac"
};

char *sEOLTypesDetailed[4] =
{
  "Auto",
  "DOS  (CR/LF)",
  "Unix (LF)",
  "Mac  (CR)"
};

TDocType DefaultDocuments[] =
{
  /* ext,               type, TabSize, UseTabs, OptimalFill, AutoIndent, BsUnindent, ThroughTabs, WordWrap */
  {"*.c;*.cpp;*.h",      tyC,       8,   FALSE,       FALSE,       TRUE,       TRUE,        TRUE,    FALSE},
  {"*.asm;*.inc",      tyASM,       8,    TRUE,        TRUE,       TRUE,       TRUE,       FALSE,    FALSE},
  {"*.a51",            tyA51,       8,    TRUE,        TRUE,       TRUE,       TRUE,       FALSE,    FALSE},
  {"*.doc;*.txt",    tyPlain,       8,   FALSE,       FALSE,      FALSE,      FALSE,        TRUE,     TRUE},
  {END_OF_DOC_LIST_CODE}
};

TDocType _DefaultDocOptions =
/* all,      type, TabSize, UseTabs, OptimalFill, AutoIndent, BsUnindent, ThroughTabs, WordWrap */
 {{'\0'}, tyPlain,       8,   TRUE,      FALSE,      FALSE,      FALSE,       FALSE,     FALSE};

const char *sNoname = "noname";

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

