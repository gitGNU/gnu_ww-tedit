/*

File: keyset.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 8th November, 1998
Descrition:
  Keyboard map definitions. Windows CUI, Borland Keyset.
  Command descriptions.

*/

#include "global.h"
#include "cmd.h"
#include "keydefs.h"
#include "kbd.h"
#include "navcmd.h"
#include "filecmd.h"
#include "blockcmd.h"
#include "editcmd.h"
#include "undocmd.h"
#include "fnavcmd.h"
#include "searcmd.h"
#include "calccmd.h"
#include "ksetcmd.h"
#include "bookmcmd.h"
#include "helpcmd.h"
#include "options.h"
#include "diag.h"
#include "keyset.h"

/*
TKeySequence should be sorted by nCode field.
If a command has two short cut key combinations the first
is displayed as short cut in the menu.
*/
TKeySequence WindowsCUI[] =
{
  {cmFileNew, DEF_KEY1(KEY(kbCtrl, kbN))},
  {cmFileOpen, DEF_KEY1(KEY(kbCtrl, kbO))},
  {cmFileSave, DEF_KEY1(KEY(kbCtrl, kbS))},
  {cmFileClose, DEF_KEY1(KEY(kbCtrl, kbF4))},
  {cmFileExit, DEF_KEY1(KEY(kbAlt, kbX))},
  {cmFileExit, DEF_KEY1(KEY(kbAlt, kbF4))},
  {cmEditCharLeft, DEF_KEY1(KEY(0, kbLeft))},
  {cmEditCharRight, DEF_KEY1(KEY(0, kbRight))},
  {cmEditLineUp, DEF_KEY1(KEY(0, kbUp))},
  {cmEditLineDown, DEF_KEY1(KEY(0, kbDown))},
  {cmEditNextWord, DEF_KEY1(KEY(kbCtrl, kbRight))},
  {cmEditPrevWord, DEF_KEY1(KEY(kbCtrl, kbLeft))},
  {cmEditHome, DEF_KEY1(KEY(0, kbHome))},
  {cmEditHome, DEF_KEY2(KEY(kbCtrl, kbK), KEY(0, kbLeft))},
  {cmEditEnd, DEF_KEY1(KEY(0, kbEnd))},
  {cmEditEnd, DEF_KEY2(KEY(kbCtrl, kbK), KEY(0, kbRight))},
  {cmEditTopFile, DEF_KEY1(KEY(kbCtrl, kbHome))},
  {cmEditBottomFile, DEF_KEY1(KEY(kbCtrl, kbEnd))},
  {cmEditPageUp, DEF_KEY1(KEY(0, kbPgUp))},
  {cmEditPageDown, DEF_KEY1(KEY(0, kbPgDn))},
  {cmEditMarkBlockBegin, DEF_KEY2(KEY(kbCtrl, kbK), KEY(0, kbB))},
  {cmEditMarkBlockEnd, DEF_KEY2(KEY(kbCtrl, kbK), KEY(0, kbK))},
  {cmEditToggleBlockHide, DEF_KEY2(KEY(kbCtrl, kbK), KEY(0, kbH))},
  {cmEditToggleBlockMarkMode, DEF_KEY1(KEY(0, kbF8))},
  {cmEditSelectAll, DEF_KEY1(KEY(kbCtrl, kbA))},
  {cmEditCharLeftExtend, DEF_KEY1(KEY(kbShift, kbLeft))},
  {cmEditCharRightExtend, DEF_KEY1(KEY(kbShift, kbRight))},
  {cmEditLineUpExtend, DEF_KEY1(KEY(kbShift, kbUp))},
  {cmEditLineDownExtend, DEF_KEY1(KEY(kbShift, kbDown))},
  {cmEditPageUpExtend, DEF_KEY1(KEY(kbShift, kbPgUp))},
  {cmEditPageDownExtend, DEF_KEY1(KEY(kbShift, kbPgDn))},
  {cmEditHomeExtend, DEF_KEY1(KEY(kbShift, kbHome))},
  {cmEditEndExtend, DEF_KEY1(KEY(kbShift, kbEnd))},
  {cmEditTopFileExtend, DEF_KEY1(KEY(kbShift + kbCtrl, kbHome))},
  {cmEditBottomFileExtend, DEF_KEY1(KEY(kbShift + kbCtrl, kbEnd))},
  {cmEditNextWordExtend, DEF_KEY1(KEY(kbShift + kbCtrl, kbRight))},
  {cmEditPrevWordExtend, DEF_KEY1(KEY(kbShift + kbCtrl, kbLeft))},
  {cmEditDeleteBlock, DEF_KEY2(KEY(kbCtrl, kbK), KEY(0, kbY))},
  {cmEditUndo, DEF_KEY1(KEY(kbCtrl, kbZ))},
  {cmEditUndo, DEF_KEY1(KEY(kbAlt, kbBckSpc))},
  {cmEditRedo, DEF_KEY1(KEY(kbCtrl, kbY))},
  {cmEditRedo, DEF_KEY1(KEY(kbAlt + kbShift, kbBckSpc))},
  {cmEditRedo, DEF_KEY1(KEY(kbCtrl + kbShift, kbZ))},
  {cmEditDel, DEF_KEY1(KEY(0, kbDel))},
  {cmEditCut, DEF_KEY1(KEY(kbCtrl, kbX))},
  {cmEditCut, DEF_KEY1(KEY(kbShift, kbDel))},
  {cmEditCopy, DEF_KEY1(KEY(kbCtrl, kbC))},
  {cmEditCopy, DEF_KEY1(KEY(kbCtrl, kbIns))},
  {cmEditPaste, DEF_KEY1(KEY(kbCtrl, kbV))},
  {cmEditPaste, DEF_KEY1(KEY(kbShift, kbIns))},
  {cmEditClipboardHistory, DEF_KEY1(KEY(kbCtrl + kbShift, kbV))},
  {cmEditBackspace, DEF_KEY1(KEY(0, kbBckSpc))},
  {cmEditIndent, DEF_KEY2(KEY(kbCtrl, kbK), KEY(0, kbI))},
  {cmEditUnindent, DEF_KEY2(KEY(kbCtrl, kbK), KEY(0, kbU))},
  {cmEditTab, DEF_KEY1(KEY(0, kbTab))},
  {cmEditTabBack, DEF_KEY1(KEY(kbShift, kbTab))},
  {cmEditIncrementalSearch, DEF_KEY1(KEY(kbCtrl, kbI))},
  {cmEditIncrementalSearchBack, DEF_KEY1(KEY(kbShift + kbCtrl, kbI))},
  {cmEditFindSelected, DEF_KEY1(KEY(kbCtrl, kbF3))},
  {cmEditFindSelectedBack, DEF_KEY1(KEY(kbCtrl + kbShift, kbF3))},
  {cmEditFindNext, DEF_KEY1(KEY(0, kbF3))},
  {cmEditFindBack, DEF_KEY1(KEY(kbShift, kbF3))},
  {cmEditFind, DEF_KEY1(KEY(kbCtrl, kbF))},
  {cmEditFind, DEF_KEY1(KEY(kbAlt, kbF3))},
  {cmEditGotoLine, DEF_KEY1(KEY(kbCtrl, kbG))},
  {cmEditEnter, DEF_KEY1(KEY(0, kbEnter))},
  {cmEditBookmarkToggle, DEF_KEY1(KEY(kbCtrl, kbF2))},
  {cmToolCalculator, DEF_KEY1(KEY(kbShift, kbF9))},
  {cmWindowSwap, DEF_KEY1(KEY(kbCtrl, kbTab))},
  {cmWindowSwap, DEF_KEY1(KEY(kbCtrl + kbShift, kbTab))},
  {cmWindowSwap, DEF_KEY1(KEY(kbCtrl, kbF6))},
  {cmWindowSwap, DEF_KEY1(KEY(kbCtrl + kbShift, kbF6))},
  {cmWindowUserScreen, DEF_KEY1(KEY(kbAlt, kbF5))},
  {cmWindowList, DEF_KEY1(KEY(kbAlt, kb0))},
  {cmWindowMessages, DEF_KEY1(KEY(kbAlt, kb2))},
  {cmWindowBookmarks, DEF_KEY1(KEY(kbAlt, kbF2))},
  {cmWindowTagNext, DEF_KEY1(KEY(0, kbF4))},
  {cmWindowTagPrev, DEF_KEY1(KEY(kbShift, kbF4))},
  {cmWindowBookmarkNext, DEF_KEY1(KEY(0, kbF2))},
  {cmWindowBookmarkPrev, DEF_KEY1(KEY(kbShift, kbF2))},
  {cmOptionsToggleInsertMode, DEF_KEY1(KEY(0, kbIns))},
  {cmOptionsToggleColumnBlock, DEF_KEY1(KEY(kbCtrl | kbShift, kbF8))},
  {cmOptionsToggleColumnBlock, DEF_KEY2(KEY(kbCtrl, kbK), KEY(0, kbN))},
  {cmHelpKbd, DEF_KEY1(KEY(0, kbF1))},
  {cmDiag, DEF_KEY2(KEY(kbCtrl, kbD), KEY(0, kbM))},

  {cmEditToggleBlockMarkMode, DEF_KEY1(KEY(kbCtrl, kbE))},
  {cmOptionsToggleColumnBlock, DEF_KEY2(KEY(kbCtrl, kbD), KEY(0, kbE))},
  {cmEditIncrementalSearch, DEF_KEY2(KEY(kbCtrl, kbD), KEY(0, kbI))},
  {cmEditFindNext, DEF_KEY1(KEY(kbCtrl, kbL))},
  {cmEditFindBack, DEF_KEY2(KEY(kbCtrl, kbD), KEY(0, kbL))},
  {cmEditNextWord, DEF_KEY2(KEY(kbCtrl, kbD), KEY(0, kbRight))},
  {cmEditPrevWord, DEF_KEY2(KEY(kbCtrl, kbD), KEY(0, kbLeft))},
  {cmEditTopFile, DEF_KEY2(KEY(kbCtrl, kbD), KEY(0, kbHome))},
  {cmEditBottomFile, DEF_KEY2(KEY(kbCtrl, kbD), KEY(0, kbEnd))},
  {cmEditBookmarkToggle, DEF_KEY2(KEY(kbCtrl, kbB), KEY(0, kbB))},
  {cmWindowBookmarkNext, DEF_KEY2(KEY(kbCtrl, kbB), KEY(0, kbDown))},
  {cmWindowBookmarkPrev, DEF_KEY2(KEY(kbCtrl, kbB), KEY(0, kbUp))},

  {END_OF_KEY_LIST_CODE}  /* LastItem */
};

TKeySet KeySet =
{
  WindowsCUI,
  _countof(WindowsCUI) - 1,
  TRUE
};

TCmdDesc Commands[] =  /* All commands in CommandsSet should be described here */
{
  {cmFileNew, "FileNew", CmdFileNew, "new", "Help on (File)~N~ew"},
  {cmFileOpen, "FileOpen", CmdFileOpen, "open", "Help on (File)~O~pen"},
  {cmFileOpenAsReadOnly, "FileOpenAsReadOnly", CmdFileOpenAsReadOnly, "open_r_only", "Help on (File)~O~open As Read Only"},
  {cmFileSave, "FileSave", CmdFileSave, "save", "Help on (File)~S~ave"},
  {cmFileSaveAs, "FileSaveAs", CmdFileSaveAs, "saveas", "Help on (File)~S~ave As"},
  {cmFileSaveAll, "FileSaveAll", CmdFileSaveAll, "saveall", "Help on (File)~S~ave All"},
  {cmFileClose, "FileClose", CmdFileClose, "close", "Help on (File)~C~lose"},
  {cmFileExit, "FileExit", CmdFileExit, "exit", "Help on (File)~E~xit"},
  {cmFileOpenMRU1, "FileOpenMRU1", CmdFileOpenMRU1, "recentfiles", "Help on (File)~R~ecent Files"},
  {cmFileOpenMRU2, "FileOpenMRU2", CmdFileOpenMRU2, "recentfiles", "Help on (File)~R~ecent Files"},
  {cmFileOpenMRU3, "FileOpenMRU3", CmdFileOpenMRU3, "recentfiles", "Help on (File)~R~ecent Files"},
  {cmFileOpenMRU4, "FileOpenMRU4", CmdFileOpenMRU4, "recentfiles", "Help on (File)~R~ecent Files"},
  {cmFileOpenMRU5, "FileOpenMRU5", CmdFileOpenMRU5, "recentfiles", "Help on (File)~R~ecent Files"},
  {cmFileOpenMRU6, "FileOpenMRU6", CmdFileOpenMRU6, "recentfiles", "Help on (File)~R~ecent Files"},
  {cmFileOpenMRU7, "FileOpenMRU7", CmdFileOpenMRU7, "recentfiles", "Help on (File)~R~ecent Files"},
  {cmFileOpenMRU8, "FileOpenMRU8", CmdFileOpenMRU8, "recentfiles", "Help on (File)~R~ecent Files"},
  {cmFileOpenMRU9, "FileOpenMRU9", CmdFileOpenMRU9, "recentfiles", "Help on (File)~R~ecent Files"},
  {cmFileOpenMRU10, "FileOpenMRU10", CmdFileOpenMRU10, "recentfiles", "Help on (File)~R~ecent Files"},
  {cmEditCharLeft, "EditCharLeft", CmdEditCharLeft, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditCharRight, "EditCharRight", CmdEditCharRight, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditLineUp, "EditLineUp", CmdEditLineUp, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditLineDown, "EditLineDown", CmdEditLineDown, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditNextWord, "EditNextWord", CmdEditNextWord, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditPrevWord, "EditPrevWord", CmdEditPrevWord, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditHome, "EditHome", CmdEditHome, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditEnd, "EditEnd", CmdEditEnd, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditTopFile, "EditTopFile", CmdEditTopFile, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditBottomFile, "EditBottomFile", CmdEditBottomFile, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditPageUp, "EditPageUp", CmdEditPageUp, "navigation", "Help on (Edit)~C~ursor navigation"},
  {cmEditPageDown, "EditPageDown", CmdEditPageDown, "navigation", "Help on (Edit)~C~ursor Navigation"},
  {cmEditMarkBlockBegin, "EditMarkBlockBegin", CmdEditMarkBlockBegin, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditMarkBlockEnd, "EditMarkBlockEnd", CmdEditMarkBlockEnd, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditToggleBlockHide, "EditToggleBlockHide", CmdEditToggleBlockHide, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditToggleBlockMarkMode, "EditToggleBlockMarkMode", CmdEditToggleBlockMarkMode, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditSelectAll, "EditSelectAll", CmdEditSelectAll, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditCharLeftExtend, "EditCharLeftExtend", CmdEditCharLeftExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditCharRightExtend, "EditCharRightExtend", CmdEditCharRightExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditLineUpExtend, "EditLineUpExtend", CmdEditLineUpExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditLineDownExtend, "EditLineDownExtend", CmdEditLineDownExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditPageUpExtend, "EditPageUpExtend", CmdEditPageUpExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditPageDownExtend, "EditPageDownExtend", CmdEditPageDownExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditHomeExtend, "EditHomeExtend", CmdEditHomeExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditEndExtend, "EditEndExtend", CmdEditEndExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditTopFileExtend, "EditTopFileExtend", CmdEditTopFileExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditBottomFileExtend, "EditBottomFileExtend", CmdEditBottomFileExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditNextWordExtend, "EditNextWordExtend", CmdEditNextWordExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditPrevWordExtend, "EditPrevWordExtend", CmdEditPrevWordExtend, "blocksel", "Help on (Edit)~B~lock Selection"},
  {cmEditDeleteBlock, "EditDeleteBlock", CmdEditDeleteBlock, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditUndo, "EditUndo", CmdEditUndo, "undo", "Help on (Edit)~U~ndo/Redo"},
  {cmEditRedo, "EditRedo", CmdEditRedo, "undo", "Help on (Edit)~U~ndo/Redo"},
  {cmEditDel, "EditDel", CmdEditDel, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditCut, "EditCut", CmdEditCut, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditCopy, "EditCopy", CmdEditCopy, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditPaste, "EditPaste", CmdEditPaste, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditClipboardHistory, "EditClipboardHistory", CmdEditClipboardHistory, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditBackspace, "EditBackspace", CmdEditBackspace, "basic", "Help on (Edit)~B~asic text input"},
  {cmEditSort, "EditSort", CmdEditSort, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditTabify, "EditTabify", CmdEditTabify, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditUntabify, "EditUntabify", CmdEditUntabify, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditUppercase, "EditUppercase", CmdEditUppercase, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditLowercase, "EditLowercase", CmdEditLowercase, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditIndent, "EditIndent", CmdEditIndent, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditUnindent, "EditUnindent", CmdEditUnindent, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditTab, "EditTab", CmdEditTab, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditTabBack, "EditTabBack", CmdEditTabBack, "block", "Help on (Edit)~B~lock Operations"},
  {cmEditTrimTrailingBlanks, "EditTrimTrailingBlanks", CmdEditTrimTrailingBlanks,  "block", "Help on (Edit)~B~lock Operations"},
  {cmEditIncrementalSearch, "EditIncrementalSearch", CmdEditIncrementalSearch, "isearch", "Help on (Edit)~I~ncremental Search"},
  {cmEditIncrementalSearchBack, "EditIncrementalSearchBack", CmdEditIncrementalSearchBack, "isearch", "Help on (Edit)~I~ncremental Search"},
  {cmEditFindSelected, "EditFindSelected", CmdEditFindSelected, "search", "Help on (Edit)~S~earch"},
  {cmEditFindSelectedBack, "EditFindSelectedBack", CmdEditFindSelectedBack, "search", "Help on (Edit)~S~earch"},
  {cmEditFindNext, "EditFindNext", CmdEditFindNext, "search", "Help on (Edit)~S~earch"},
  {cmEditFindBack, "EditFindBack", CmdEditFindBack, "search", "Help on (Edit)~S~earch"},
  {cmEditFind, "EditFind", CmdEditFind, "search", "Help on (Edit)~S~earch"},
  {cmEditGotoLine, "EditGotoLine", CmdEditGotoLine, "gotoln", "Help on (Edit)~G~oto line"},
  {cmEditFindInFiles, "EditFindInFiles", CmdEditFindInFiles, "findinfiles", "Help on (Edit)~F~ind/Replace In Files"},
  {cmEditSearchFunctions, "EditSearchFunctions", CmdEditSearchFunctions, "searchfunctions", "Help on (Edit)Search ~F~unctions names"},
  {cmEditEnter, "EditEnter", CmdEditEnter, "basic", "Help on (Edit)~B~asic text input"},
  {cmEditBookmarkToggle, "EditBookmarkToggle", CmdEditBookmarkToggle, "bookmarks", "Help on (Edit)~B~ookmarks"},
  {cmToolCalculator, "ToolCalculator", CmdToolCalculator, "calc", "Help on (Tools)Calculator"},
  {cmToolParseLogFile, "ToolParseLogFile", CmdToolParseLogFile, "parselogfile", "Help on (Tools)Parse Log File"},
  {cmWindowSwap, "WindowSwap", CmdWindowSwap, "wnavigation", "Help on (Windows)~N~avigation"},
  {cmWindowUserScreen, "WindowUserScreen", CmdWindowUserScreen, "userscr", "Help on (Windows)~U~ser screeen"},
  {cmWindowList, "WindowList", CmdWindowList, "wlist", "Help on (Windows)~L~ist"},
  {cmWindow1, "Window1", CmdWindow1, "wlist", "Help on (Windows)~L~ist"},
  {cmWindow2, "Window2", CmdWindow2, "wlist", "Help on (Windows)~L~ist"},
  {cmWindow3, "Window3", CmdWindow3, "wlist", "Help on (Windows)~L~ist"},
  {cmWindow4, "Window4", CmdWindow4, "wlist", "Help on (Windows)~L~ist"},
  {cmWindow5, "Window5", CmdWindow5, "wlist", "Help on (Windows)~L~ist"},
  {cmWindow6, "Window6", CmdWindow6, "wlist", "Help on (Windows)~L~ist"},
  {cmWindow7, "Window7", CmdWindow7, "wlist", "Help on (Windows)~L~ist"},
  {cmWindow8, "Window8", CmdWindow8, "wlist", "Help on (Windows)~L~ist"},
  {cmWindow9, "Window9", CmdWindow9, "wlist", "Help on (Windows)~L~ist"},
  {cmWindow10, "Window10", CmdWindow10, "wlist", "Help on (Windows)~L~ist"},
  {cmWindowMessages, "WindowMessages", CmdWindowMessages, "messages", "Help on (Window)~M~essages output"},
  {cmWindowBookmarks, "WindowBookmarks", CmdWindowBookmarks, "messages", "Help on (Window)~M~essages output"},
  {cmWindowFindInFiles1, "WindowFindInFiles1", CmdWindowFindInFiles1, "messages", "Help on (Window)~M~essages output"},
  {cmWindowFindInFiles2, "WindowFindInFiles2", CmdWindowFindInFiles2, "messages", "Help on (Window)~M~essages output"},
  {cmWindowOutput, "WindowOutput", CmdWindowOutput, "output", "Help on (Window)~M~essages output"},
  {cmWindowTagNext, "WindowTagNext", CmdWindowTagNext, "messages", "Help on (Window)~M~essages output"},
  {cmWindowTagPrev, "WindowTagPrev", CmdWindowTagPrev, "messages", "Help on (Window)~M~essages output"},
  {cmWindowBookmarkNext, "WindowBookmarkNext", CmdWindowBookmarkNext, "bookmarks", "Help on (Edit)~B~ookmarks"},
  {cmWindowBookmarkPrev, "WindowBookmarkPrev", CmdWindowBookmarkPrev, "bookmarks", "Help on (Edit)~B~ookmarks"},
  {cmOptionsSave, "OptionsSave", CmdOptionsSave},
  {cmOptionsToggleInsertMode, "OptionsToggleInsertMode", CmdOptionsToggleInsertMode},
  {cmOptionsToggleColumnBlock, "OptionsToggleColumnBlock", CmdOptionsToggleColumnBlock},
  {cmHelpEditor, "HelpEditor", CmdHelpEditor},
  {cmHelpHistory, "HelpHistory", CmdHelpHistory},
  {cmHelpOpenFile, "HelpOpenFile", CmdHelpOpenFile},
  {cmHelpKbd, "HelpKbd", CmdHelpKbd},
  {cmHelpAbout, "HelpAbout", CmdHelpAbout},
  {cmDiag, "Diag", CmdDiag},
  {END_OF_CMD_LIST_CODE, "End_Of_Cmd_List_Code"}  /* LastItem */
};

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

