/*

File: palette.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 9th November, 1998
Descrition:
  Collor palette.
  Borland palette, Borland conservative palette, Dark palette,
  BW palette, Classic termina palette.

*/

#include "global.h"
#include "palette.h"

BYTE BorlandPalette[MAX_PALETTE] =
{
  0x70, 0x4f,  /* coStatus, coError */
  0x4f, 0x4e,  /* coTabs, coReadOnly */
  0x71,        /* coRecStored */
  0x70, 0x74,  /* coStatusTxt, coStatusShortCut */
  #ifdef DARK_BPAL
  0x0f, 0x0e,  /* coEOF, coEdText */
  0x70, 0x0e,  /* coBlock, coEdNumber */
  0x07, 0x0f,  /* coEdComment, coEdReserved */
  0x0a, 0x0f,  /* coEdRegister, coEdInstruction */
  0x0e, 0x0a,  /* coEdString, coEdPreproc */
  0x0f, 0x0b,  /* coEdOper, coEdSFR */
  0x0a,        /* coEdPair */
  #else
  0x1f, 0x1e,  /* coEOF, coEdText */
  0x70, 0x1e,  /* coBlock, coEdNumber */
  0x17, 0x1f,  /* coEdComment, coEdReserved */
  0x1a, 0x1f,  /* coEdRegister, coEdInstruction */
  0x1e, 0x1a,  /* coEdString, coEdPreproc */
  0x1f, 0x1b,  /* coEdOper, coEdSFR */
  0x1a,        /* coEdPair */
  #endif
  0x30,        /* coEdTooltip */
  0x60,        /* coEdBlockCursor */
  0x20,        /* coEdBookmark */
  0x0f, 0x0f,  /* coSmallEOF, coSmallEdText */
  0x70, 0x0f,  /* coSmallEdBlock, coSmallEdNumber */
  0x0f, 0x0f,  /* coSmallEditEdComment, coSmallEditEdReserved */
  0x0f, 0x0f,  /* coSmallEditEdRegister, coSmallEditEdInstruction */
  0x0f, 0x0f,  /* coSmallEditEdString, coSmallEditEdPreproc */
  0x0f, 0x0a,  /* coSmallEditEdOper, coSmallEditEdSFR */
  0x0f,        /* coSmallEditEdPair */
  0x10,        /* coSmallEdTooltip */
  0x60,        /* coSmallEdBlockCursor */
  0x02,        /* coSmallEdBookmark */
  0x0a,        /* coEnterLn */
  0x20, 0x0b,  /* coUnchanged, coEnterLnPrompt */
  0x02,	       /* coEnterLnBrace */
  0x70, 0x70,  /* coUMenuFrame, coUMenuItems */
  0x20, 0x70,  /* coUMenuSelected, coUMenuTitle */
  0x30, 0x30,  /* coHelpFrame, coHelpItems */
  0x3e, 0x30,  /* coHelpSelected, coHelpTitle */
#if 0
  0x3f,        /* coHelpText (infordr) */
  0x30,        /* coHelpLink (infordr) */
  0x3e,        /* coHelpHighlightLink (infordr) */
  0x37,        /* coHelpHighlightMenu (infordr) */
  0x17,        /* coHelpTextSelected (infordr) */
#endif
  0x07,        /* coHelpText (infordr) */
  0x0f,        /* coHelpLink (infordr) */
  0x30,        /* coHelpHighlightLink (infordr) */
  0x02,        /* coHelpHighlightMenu (infordr) */
  0x70,        /* coHelpTextSelected (infordr) */
  0x70, 0x74,  /* coMenu, coMenuShortCut */
  0x20, 0x24,  /* coMenuSelected, coMenuSeShortCut */
  0x78, 0x70,  /* coMenuDisabled, coMenuFrame */
  0x08,        /* coMenuSeDisabl */
  0x3f, 0x30,  /* coCtxHlpMenu, coCtxHlpMenuShortCut */
  0x0e, 0x07,  /* coCtxHlpMenuSelected, coCtxHlpMenuSeShortCt */
  0x37, 0x30,  /* coCtxHlpMenuDisabled, coCtxHlpMenuFrame */
  0x3e,        /* coCtxHlpMenuSeDisabl */
  0x02, 0x0a   /* coTerm1, coTerm2 */
};

BYTE VCPalette[MAX_PALETTE] =
{
  0x70, 0x4f,  /* coStatus, coError */
  0x4f, 0x4e,  /* coTabs, coReadOnly */
  0x71,        /* coRecStored */
  0x70, 0x74,  /* coStatusTxt, coStatusShortCut */
  0xF0, 0xF0,  /* coEOF, coEdText */
  0x0F, 0xF0,  /* coEdBlock, coEdNumber */
  0xF2, 0xF9,  /* coEdComment, coEdReserved */
  0xF0, 0xF9,  /* coEdRegister, coEdInstruction */
  0xF0, 0xF9,  /* coEdString, coEdPreproc */ 
#ifdef WIN32
  0xF0, 0xF6,  /* coEdOper, coEdSFR */
#else
  0xF0, 0xF4,  /* coEdOper, coEdSFR */
#endif
  0xF3,        /* coEdPair */
  0xe0,        /* coEdTooltip */
  0x60,        /* coEdBlockCursor */
  0x02,        /* coEdBookmark */
  0x0f, 0x0f,  /* coSmallEOF, coSmallEdText */
  0x70, 0x0f,  /* coSmallEdBlock, coSmallEdNumber */
  0x0f, 0x0f,  /* coSmallEditEdComment, coSmallEditEdReserved */
  0x0f, 0x0f,  /* coSmallEditEdRegister, coSmallEditEdInstruction */
  0x0f, 0x0f,  /* coSmallEditEdString, coSmallEditEdPreproc */
  0x0f, 0x0a,  /* coSmallEditEdOper, coSmallEditEdSFR */
  0x0f,        /* coSmallEditEdPair */
  0xe0,        /* coSmallEdTooltip */
  0x60,        /* coSmallEdBlockCursor */
  0x02,        /* coSmallEdBookmark */
  0x0a,        /* coEnterLn */
  0x20, 0x0b,  /* coUnchanged, coEnterLnPrompt */
  0x02,	       /* coEnterLnBrace */
  0x70, 0x70,  /* coUMenuFrame, coUMenuItems */
  0x20, 0x70,  /* coUMenuSelected, coUMenuTitle */
  0x30, 0x30,  /* coHelpFrame, coHelpItems */
  0x3e, 0x30,  /* coHelpSelected, coHelpTitle */
#if 0
  0x3f,        /* coHelpText (infordr) */
  0x30,        /* coHelpLink (infordr) */
  0x3e,        /* coHelpHighlightLink (infordr) */
  0x37,        /* coHelpHighlightMenu (infordr) */
  0x17,        /* coHelpTextSelected (infordr) */
#endif
  0x07,        /* coHelpText (infordr) */
  0x0f,        /* coHelpLink (infordr) */
  0x30,        /* coHelpHighlightLink (infordr) */
  0x02,        /* coHelpHighlightMenu (infordr) */
  0x70,        /* coHelpTextSelected (infordr) */
  0x70, 0x74,  /* coMenu, coMenuShortCut */
  0x20, 0x24,  /* coMenuSelected, coMenuSeShortCut */
  0x78, 0x70,  /* coMenuDisabled, coMenuFrame */
  0x08,        /* coMenuSeDisabl */
  0x3f, 0x30,  /* coCtxHlpMenu, coCtxHlpMenuShortCut */
  0x0e, 0x07,  /* coCtxHlpMenuSelected, coCtxHlpMenuSeShortCt */
  0x37, 0x30,  /* coCtxHlpMenuDisabled, coCtxHlpMenuFrame */
  0x3e,        /* coCtxHlpMenuSeDisabl */
  0x02, 0x0a   /* coTerm1, coTerm2 */
};

#ifdef _NON_TEXT
//BYTE *CPalette = BorlandPalette;        /* Current palette */
BYTE *CPalette = VCPalette;        /* Current palette */
#else
BYTE *CPalette = BorlandPalette;        /* Current palette */
#endif

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

