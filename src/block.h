/*

File: block.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 31st March, 1999
Descrition:
  Functions concerning text block storage and manipulation.

*/

#ifndef BLOCK_H
#define BLOCK_H

#include "tarray.h"
#include "tblocks.h"
#include "file.h"

TBlock *AllocTBlock(void);
void FreeTBlock(TBlock *pBlock);

TLine *GetBlockLine(const TBlock *pBlock, int nLine);
#define GetBlockLineText(pBlock, nLine)  (GetBlockLine(pBlock, nLine)->pLine)

char *tblock_copy_to_str(TBlock *pblock, int dest_eol_type,
  char *dest, int dest_buf_size);
void MarkBlockBegin(TFile *pFile);
void MarkBlockEnd(TFile *pFile);
BOOLEAN ValidBlockMarkers(TFile *pFile);
void ToggleBlockHide(TFile *pFile);
void BeginBlockExtend(TFile *pFile);
void EndBlockExtend(TFile *pFile);

void DisposeABlock(TBlock **pBlock);
BOOLEAN DetabColumnRegion(TFile *pFile,
  int nStartLine, int nStartCol, int nEndLine, int nEndCol);
TBlock *MakeBlock(const char *s, WORD blockattr);
TBlock *MakeACopyOfBlock(const TFile *pFile,
  int nStartLine, int nEndLine, int nStartPos, int nEndPos,
  WORD blockattr);
TBlock *DuplicateBlock(const TBlock *pBlock1, int nNewEOLType,
  int nPrefixSize, int nSuffixSize, int *pnSuffixPos, int *pnLastLineLen);
#define ExactBlockCopy(pBlock)  DuplicateBlock(pBlock, pFile->nEOLType, 0, 0, NULL, NULL);

/*
Text insertion/deletion primitives.
Used by cut/paste operations.
Used by undo/redo operations.
Notes:
Each primitive has its undo or redo counterpart.
Column block primitives operate with patterns only.
*/
BOOLEAN InsertCharacterBlockPrim(TFile *pFile, const TBlock *pBlock);
BOOLEAN InsertColumnBlockPrim(TFile *pFile, const TBlock *pBlockPattern);
BOOLEAN DeleteColumnBlockPrim(TFile *pFile,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos,
  BOOLEAN bGeneratePattern, TBlock **pBlockPattern);
BOOLEAN DeleteCharacterBlockPrim(TFile *pFile,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos);
BOOLEAN DeleteCharacterBlock(TFile *pFile,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos);
BOOLEAN DeleteColumnBlock(TFile *pFile,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos);

BOOLEAN InsertBlanks(TFile *pFile, int nLine, int nFromCol, int nToCol);

BOOLEAN PrepareColRowPosition(TFile *pFile, const TBlock *pBlock);

void DeleteACharacter(TFile *pFile);
BOOLEAN DeleteBlock(TFile *pFile);

void ReplaceTextPrim(TFile *pFile, const char *pText, char *pOldText);

void Cut(TFile *pFile, TBlock **pBlock);
void Copy(TFile *pFile, TBlock **pBlock);
BOOLEAN Paste(TFile *pFile, const TBlock *pBlock);
BOOLEAN PasteAndIndent(TFile *pFile, const TBlock *pBlock);
void Overwrite(TFile *pFile, const TBlock *pBlock);

void IndentBlock(TFile *pFile, int nIndent);
void UnindentBlock(TFile *pFile, int nUnindent);

BOOLEAN AddCharLine(TFile *pFile, const char *psLine);

void Rearrange(TFile *pFile, int nNumberOfLines, int *pLines);
void RevertRearrange(TFile *pFile, int nNumberOfLines, int *pLines);
void SortCurrentBlock(TFile *pFile);

BOOLEAN TrimTrailingBlanks(TFile *pFile, int nStartLine, int nEndLine,
  int *pnBlanksRemoved);

#endif  /* ifndef BLOCK_H */

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

