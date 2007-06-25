/*

File: tblocks.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Block list manipulation routines.

*/

#ifndef TBLOCKS_H
#define TBLOCKS_H

#include "clist.h"

typedef struct FileBlock
{
  TListEntry link;  /* All blocks are in double-linked list */
  int nRef;  /*	Pointers referencing inside this block */
  int nBlockSize;  /* Only with control purposes */
  int nFreeSize;
  /* Following is the actual block DATA */
} TFileBlock;

BOOLEAN AddBlock(TListRoot *blist, int bsize);
void AddBlockLink(TListRoot *blist, char *b);
char *AllocateTBlock(int bsize);
void DecRef(char *b, int n);
void DisposeBlock(char *b);
void DisposeBlockList(TListRoot *blist);
#define GetLastBlock(broot)  (char *)((TFileBlock *)((broot)->Blink) + 1)
#define	IncRef(b, n)\
        {\
          ASSERT((n) > 0);\
            (((TFileBlock *)(b) - 1)->nRef += n);\
        }

#define GetTBlockSize(pblock)  (((TFileBlock *)(pblock) - 1)->nBlockSize)
#define GetTBlockFree(pblock)  (((TFileBlock *)(pblock) - 1)->nFreeSize)
#define SetTBlockFree(pblock, nNewFreeSize)  ((TFileBlock *)(pblock) - 1)->nFreeSize = nNewFreeSize

#endif	/* ifdef TBLOCKS_H */

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

