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
#include "disp.h"
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

#ifdef DISP_WIN32_GUIEMU
//BYTE *CPalette = BorlandPalette;        /* Current palette */
BYTE *CPalette = VCPalette;        /* Current palette */
#else
BYTE *CPalette = BorlandPalette;        /* Current palette */
#endif

#include "disp.h"

unsigned int ww_pal[MAX_PALETTE];

int pal_init(dispc_t *disp)
{
#define PAL_ADD(idx, b, c, style) \
  if (!disp_pal_add(disp, \
                   disp_pal_get_standard(disp, c), \
                   disp_pal_get_standard(disp, b), \
                   style, &ww_pal[idx])) \
    return 0;

  PAL_ADD(coStatus, 0x7, 0x0, 0);
  PAL_ADD(coError, 0x4, 0xf, 0);
  PAL_ADD(coTabs, 0x4, 0xf, 0);
  PAL_ADD(coReadOnly, 0x4, 0xe, 0);
  PAL_ADD(coRecStored, 0x7, 0x1, 0);
  PAL_ADD(coStatusTxt, 0x7, 0x0, 0);
  PAL_ADD(coStatusShortCut, 0x7, 0x4, 0);

  #ifdef DARK_BPAL
  PAL_ADD(coEOF, 0x0, 0xf, 0);
  PAL_ADD(coEdText, 0x0, 0xe, 0);
  PAL_ADD(coEdBlock, 0x7, 0x0, 0);
  PAL_ADD(coEdNumber, 0x0, 0xe, 0);
  PAL_ADD(coEdComment, 0x0, 0x7, 0);
  PAL_ADD(coEdReserved, 0x0, 0xf, 0);
  PAL_ADD(coEdRegister, 0x0, 0xa, 0);
  PAL_ADD(coEdInstruction, 0x0, 0xf, 0);
  PAL_ADD(coEdString, 0x0, 0xe, 0);
  PAL_ADD(coEdPreproc, 0x0, 0xa, 0);
  PAL_ADD(coEdOper, 0x0, 0xf, 0);
  PAL_ADD(coEdSFR, 0x0, 0xb, 0);
  PAL_ADD(coEdPair, 0x0, 0xa, 0);
  #else
  PAL_ADD(coEOF, 0x1, 0xf, 0);
  PAL_ADD(coEdText, 0x1, 0xe, 0);
  PAL_ADD(coEdBlock, 0x7, 0x0, 0);
  PAL_ADD(coEdNumber, 0x1, 0xe, 0);
  PAL_ADD(coEdComment, 0x1, 0x7, 0);
  PAL_ADD(coEdReserved, 0x1, 0xf, 0);
  PAL_ADD(coEdRegister, 0x1, 0xa, 0);
  PAL_ADD(coEdInstruction, 0x1, 0xf, 0);
  PAL_ADD(coEdString, 0x1, 0xe, 0);
  PAL_ADD(coEdPreproc, 0x1, 0xa, 0);
  PAL_ADD(coEdOper, 0x1, 0xf, 0);
  PAL_ADD(coEdSFR, 0x1, 0xb, 0);
  PAL_ADD(coEdPair, 0x1, 0xa, 0);
  #endif

  PAL_ADD(coEdTooltip, 0x3, 0x0, 0);
  PAL_ADD(coEdBlockCursor, 0x6, 0x0, 0);
  PAL_ADD(coEdBookmark, 0x2, 0x0, 0);
  PAL_ADD(coSmallEdEOF, 0x0, 0xf, 0);
  PAL_ADD(coSmallEdText, 0x0, 0xf, 0);
  PAL_ADD(coSmallEdBlock, 0x7, 0x0, 0);
  PAL_ADD(coSmallEdNumber, 0x0, 0xf, 0);
  PAL_ADD(coSmallEditEdComment, 0x0, 0xf, 0);
  PAL_ADD(coSmallEditEdReserved, 0x0, 0xf, 0);
  PAL_ADD(coSmallEditEdRegister, 0x0, 0xf, 0);
  PAL_ADD(coSmallEditEdInstruction, 0x0, 0xf, 0);
  PAL_ADD(coSmallEditEdString, 0x0, 0xf, 0);
  PAL_ADD(coSmallEditEdPreproc, 0x0, 0xf, 0);
  PAL_ADD(coSmallEditEdOper, 0x0, 0xf, 0);
  PAL_ADD(coSmallEditEdSFR, 0x0, 0xa, 0);
  PAL_ADD(coSmallEditEdPair, 0x0, 0xf, 0);
  PAL_ADD(coSmallEdTooltip, 0x1, 0x0, 0);
  PAL_ADD(coSmallEdBlockCursor, 0x6, 0x0, 0);
  PAL_ADD(coSmallEdBookmark, 0x0, 0x2, 0);
  PAL_ADD(coEnterLn, 0x0, 0xa, 0);
  PAL_ADD(coUnchanged, 0x2, 0x0, 0);
  PAL_ADD(coEnterLnPrompt, 0x0, 0xb, 0);
  PAL_ADD(coEnterLnBraces, 0x0, 0x2, 0);
  PAL_ADD(coUMenuFrame, 0x7, 0x0, 0);
  PAL_ADD(coUMenuItems, 0x7, 0x0, 0);
  PAL_ADD(coUMenuSelected, 0x2, 0x0, 0);
  PAL_ADD(coUMenuTitle, 0x7, 0x0, 0);
  PAL_ADD(coHelpFrame, 0x3, 0x0, 0);
  PAL_ADD(coHelpItems, 0x3, 0x0, 0);
  PAL_ADD(coHelpSelected, 0x3, 0xe, 0);
  PAL_ADD(coHelpTitle, 0x3, 0x0, 0);

  PAL_ADD(coMenu, 0x7, 0x0, 0);
  PAL_ADD(coMenuShortCut, 0x7, 0x4, 0);
  PAL_ADD(coMenuSelected, 0x2, 0x0, 0);
  PAL_ADD(coMenuSeShortCt, 0x2, 0x4, 0);
  PAL_ADD(coMenuDisabled, 0x7, 0x8, 0);
  PAL_ADD(coMenuFrame, 0x7, 0x0, 0);
  PAL_ADD(coMenuSeDisabl, 0x0, 0x8, 0);
  PAL_ADD(coCtxHlpMenu, 0x3, 0xf, 0);
  PAL_ADD(coCtxHlpMenuShortCut, 0x3, 0x0, 0);
  PAL_ADD(coCtxHlpMenuSelected, 0x0, 0xe, 0);
  PAL_ADD(coCtxHlpMenuSeShortCt, 0x0, 0x7, 0);
  PAL_ADD(coCtxHlpMenuDisabled, 0x3, 0x7, 0);
  PAL_ADD(coCtxHlpMenuFrame, 0x3, 0x0, 0);
  PAL_ADD(coCtxHlpMenuSeDisabl, 0x3, 0xe, 0);
  PAL_ADD(coTerm1, 0x0, 0x2, 0);
  PAL_ADD(coTerm2, 0x0, 0xa, 0);

  return 1;
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

