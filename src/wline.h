/*

File: wline.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 2nd November, 1998
Descrition:
  Single line output (in buffer). Syntax highlighting.

*/

#ifndef WLINE_H
#define WLINE_H

#include "file.h"
#include "synh.h"

/* ATTENTION: any changes here should be reflected in synh.h also! */
enum SyntaxAttr
{
  attrEOF = 0,  /* <End of file> message */
  attrText,     /* Simple text */
  attrBlock,     /* Blocked text */
  attrNumber,
  attrComment,
  attrReserved,
  attrRegister,
  attrInstruction,
  attrString,
  attrPreproc,
  attrOper,
  attrSFR,
  attrPair,
  attrTooltip,
  attrBlockCursor,
  attrBookmark
};

typedef struct LineOutput
{
  char c;
  char t;  /* Syntax highlithing type index */
} TLineOutput;

DWORD GetEOLStatus(TFile *pFile, int Line);

void wline(const TFile *pFile, int nWrtLine, int nWidth, TLineOutput *pOutputBuf,
  TExtraColorInterf *pExtraColorInterf);

int FunctionNameScan(const TFile *pFile,
  int nStartLine, int nStartPos,
  int nNumLines, int nEndLine,
  TFunctionName FuncNames[], int nMaxEntries);

#endif  /* ifndef WLINE_H */

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2002, 2003
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

