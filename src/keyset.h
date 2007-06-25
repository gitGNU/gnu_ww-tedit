/*

File: keyset.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 8th November, 1998
Descrition:
  Keyboard map definitions. Windows CUI, Borland Keyset.
  Command descriptions.

*/

#ifndef KEYSET_H
#define KEYSET_H

#include "global.h"
#include "cmd.h"

enum CommandsSet {
  cmFileNew = 1,
  cmFileOpen,
  cmFileOpenAsReadOnly,
  cmFileSave,
  cmFileSaveAs,
  cmFileSaveAll,
  cmFileClose,
  cmFileExit,

  cmFileOpenMRU1,
  cmFileOpenMRU2,
  cmFileOpenMRU3,
  cmFileOpenMRU4,
  cmFileOpenMRU5,
  cmFileOpenMRU6,
  cmFileOpenMRU7,
  cmFileOpenMRU8,
  cmFileOpenMRU9,
  cmFileOpenMRU10,

  cmEditCharLeft,
  cmEditCharRight,
  cmEditLineUp,
  cmEditLineDown,
  cmEditNextWord,
  cmEditPrevWord,
  cmEditHome,
  cmEditEnd,
  cmEditTopFile,
  cmEditBottomFile,
  cmEditPageUp,
  cmEditPageDown,

  cmEditMarkBlockBegin,
  cmEditMarkBlockEnd,
  cmEditToggleBlockHide,
  cmEditToggleBlockMarkMode,
  cmEditSelectAll,
  cmEditCharLeftExtend,
  cmEditCharRightExtend,
  cmEditLineUpExtend,
  cmEditLineDownExtend,
  cmEditPageUpExtend,
  cmEditPageDownExtend,
  cmEditHomeExtend,
  cmEditEndExtend,
  cmEditTopFileExtend,
  cmEditBottomFileExtend,
  cmEditNextWordExtend,
  cmEditPrevWordExtend,
  cmEditDeleteBlock,
  cmEditUndo,
  cmEditRedo,
  cmEditDel,
  cmEditCut,
  cmEditCopy,
  cmEditPaste,
  cmEditClipboardHistory,
  cmEditBackspace,

  cmEditSort,
  cmEditTabify,
  cmEditUntabify,
  cmEditUppercase,
  cmEditLowercase,
  cmEditIndent,
  cmEditUnindent,
  cmEditTab,
  cmEditTabBack,
  cmEditTrimTrailingBlanks,

  cmEditIncrementalSearch,
  cmEditIncrementalSearchBack,
  cmEditFindSelected,
  cmEditFindSelectedBack,
  cmEditFindNext,
  cmEditFindBack,
  cmEditFind,

  cmEditFindInFiles,
  cmEditSearchFunctions,

  cmEditGotoLine,

  cmEditEnter,

  cmEditBookmarkToggle,

  cmToolCalculator,
  cmToolParseLogFile,

  cmWindowSwap,
  cmWindowUserScreen,
  cmWindowList,
  cmWindow1,
  cmWindow2,
  cmWindow3,
  cmWindow4,
  cmWindow5,
  cmWindow6,
  cmWindow7,
  cmWindow8,
  cmWindow9,
  cmWindow10,

  cmWindowMessages,
  cmWindowBookmarks,
  cmWindowFindInFiles1,
  cmWindowFindInFiles2,
  cmWindowOutput,
  cmWindowTagNext,
  cmWindowTagPrev,
  cmWindowBookmarkNext,
  cmWindowBookmarkPrev,

  cmOptionsSave,
  cmOptionsToggleInsertMode,
  cmOptionsToggleColumnBlock,

  cmHelpEditor,
  cmHelpHistory,
  cmHelpOpenFile,
  cmHelpKbd,
  cmHelpAbout,

  cmDiag,
  MAX_CMD_CODE = cmDiag + 100  /* leave room for some custom commands */
};

extern TKeySequence WindowsCUI[];
extern TKeySet KeySet;  /* All key references are pointed here */
extern TCmdDesc Commands[];  /* All commands in CommandsSet should be described here */

#endif  /* ifndef KEYSET_H */

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

