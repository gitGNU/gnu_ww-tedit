/*

File: nav.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 30th October, 1998
Descrition:
  File navigation, maintaining current position, current page updating,
  tab character manipulation functions.

*/

#ifndef NAV_H
#define NAV_H

#include "file.h"
#include "search.h"
#include "wline.h"
#include "disp.h"

void FixWrtPos(int *pnWrtPos, int nCurPos, int nWidth);
int UpdatePage(TFile *pFile, int nStartX, int nStartY, int nWinWidth, int nWinHeight,
  int nPaletteStart,
  const TSearchContext *pstSearchContext,
  int (*PutText)(int nStartX, int nStartY, int nWidth, int nYLine,
  int nPaletteStart, const TLineOutput *pBuf, dispc_t *disp),
  TExtraColorInterf *pExtraColorInterf, dispc_t *disp);
BOOLEAN AllocOutputBuffer(void);
void DisposeOutputBuffer(void);
int CalcTab(int nPos);
int LineGetTabPos(const char *pLine, int nPos);
int GetTabPos(const TFile *pFile, int nLine, int nPos);
void GotoWord(char *sLine, int nLnLen, int *nPos, int nDirection, BOOLEAN *bGotoLine,
  char *sSuplSet, BOOLEAN bGotoEndOfWord);
void GotoNextWordOrWordEnd(TFile *pFile);
void GetWordUnderCursor(const TFile *pFile, char *psDest, int nDestBufSize);
BOOLEAN CurPosInAlpha(char *pCurPos, char *sSuplSet);
void FindPos(TFile *pFile);
void GotoColRow(TFile *pFile, int nCol, int nRow);
void GotoPosRow(TFile *pFile, int nPos, int nRow);
void GotoPage(TFile *pFile, int nWinHeight, BOOLEAN bPageUp);
void LeapThroughSelection(TFile *pFile, BOOLEAN bForward);

void CharLeft(TFile *pFile);
void CharRight(TFile *pFile);
void LineUp(TFile *pFile);
void LineDown(TFile *pFile);
void GotoNextWord(TFile *pFile);
void GotoPrevWord(TFile *pFile);
void GotoHomePosition(TFile *pFile);
void GotoEndPosition(TFile *pFile);
void GotoTop(TFile *pFile);
void GotoBottom(TFile *pFile);
void GotoPageUp(TFile *pFile, int nPageSize);
void GotoPageDown(TFile *pFile, int nPageSize);

#endif  /* ifndef NAV_H */

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

