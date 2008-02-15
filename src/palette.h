/*

File: palette.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 9th November, 1998
Descrition:
  Collor palette.
  Borland palette, Borland conservative palette, Dark palette,
  BW palette, Classic termina palette.

*/

#ifndef	PALETTE_H
#define PALETTE_H

enum Colors
{
  coStatus = 0,
  coError,
  coTabs,
  coReadOnly,
  coRecStored,
  coStatusTxt,
  coStatusShortCut,
  coEOF,
  coEdText,
  coEdBlock,
  coEdNumber,
  coEdComment,
  coEdReserved,
  coEdRegister,
  coEdInstruction,
  coEdString,
  coEdPreproc,
  coEdOper,
  coEdSFR,
  coEdPair,
  coEdTooltip,
  coEdBlockCursor,
  coEdBookmark,
  coSmallEdEOF,
  coSmallEdText,
  coSmallEdBlock,
  coSmallEdNumber,
  coSmallEditEdComment,
  coSmallEditEdReserved,
  coSmallEditEdRegister,
  coSmallEditEdInstruction,
  coSmallEditEdString,
  coSmallEditEdPreproc,
  coSmallEditEdOper,
  coSmallEditEdSFR,
  coSmallEditEdPair,
  coSmallEdTooltip,
  coSmallEdBlockCursor,
  coSmallEdBookmark,
  coEnterLn,
  coUnchanged,
  coEnterLnPrompt,
  coEnterLnBraces,
  coUMenuFrame,
  coUMenuItems,
  coUMenuSelected,
  coUMenuTitle,
  coHelpFrame,
  coHelpItems,
  coHelpSelected,
  coHelpTitle,
  coHelpText,
  coHelpLink,
  coHelpHighlightLink,
  coHelpHighlightMenu,
  coHelpTextSelected,
  coMenu,
  coMenuShortCut,
  coMenuSelected,
  coMenuSeShortCt,
  coMenuDisabled,
  coMenuFrame,
  coMenuSeDisabl,
  coCtxHlpMenu,
  coCtxHlpMenuShortCut,
  coCtxHlpMenuSelected,
  coCtxHlpMenuSeShortCt,
  coCtxHlpMenuDisabled,
  coCtxHlpMenuFrame,
  coCtxHlpMenuSeDisabl,
  coTerm1,
  coTerm2,
  MAX_PALETTE
};

#define  SyntaxPaletteStart  coEOF
#define  SmallEditorPaletteStart coSmallEdEOF
#define  coUMenu  coUMenuFrame
#define  coHelpList coHelpFrame

#define _coUMenuFrame	0
#define _coUMenuItems	1
#define _coUMenuSelected	2
#define _coUMenuTitle	3

#define _co_Menu 0
#define _co_MenuShortCut 1
#define _co_MenuSelected 2
#define _co_MenuSeShortCt 3
#define _co_MenuDisabled 4
#define _co_MenuFrame 5
#define _co_MenuSeDisabl 6

extern unsigned int ww_pal[];
#define GetColor(c) (ww_pal[(c)])

#include "disp.h"

int pal_init(dispc_t *disp);

#endif  /* ifndef PALETTE_H */

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

