/*

File: tblocks.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Block list manipulation routines.

*/

#include "global.h"
#include "memory.h"
#include "tblocks.h"

/* ************************************************************************
   Function: AddBlock
   Description:
     Adds a block to block list.
   Parameters:
     blist -- the root of a block list (TListRoot)
     b -- points to a block to be added
*/
BOOLEAN AddBlock(TListRoot *blist, int bsize)
{
  TFileBlock *b;

  ASSERT(bsize > 0);

  b = alloc(bsize + sizeof(TFileBlock));
  if (b == NULL)
    return FALSE;

  b->nRef = 0;
  b->nBlockSize = bsize;
  b->nFreeSize = bsize;
  INSERT_TAIL_LIST(blist, &b->link);

  return TRUE;
}

/* ************************************************************************
   Function: AddBlockLink
   Description:
     Adds an already allocated block to blist.
*/
void AddBlockLink(TListRoot *blist, char *b)
{
  TFileBlock *b2;

  ASSERT(b != NULL);

  b2 = (TFileBlock *)b - 1;
  INSERT_TAIL_LIST(blist, &b2->link);
}

/* ************************************************************************
   Function: AllocateBlock
   Description:
     This function is to be used when only a single block
     is necessary to be allocated. DecRef() coule be used to keep
     count of the block references and DisposeBlock() could
     be used for final block disposal.
*/
char *AllocateTBlock(int bsize)
{
  TFileBlock *b;

  ASSERT(bsize > 0);

  b = alloc(bsize + sizeof(TFileBlock));
  if (b == NULL)
    return NULL;

  b->nRef = 0;
  b->nBlockSize = bsize;
  INITIALIZE_LIST_HEAD(&b->link);  /* Single block only */

  return (char *)(b + 1);
}

/* ************************************************************************
   Function: DecRef
   Description:
     Decrements the reference counter of a block.
     Disposes the block if its reference counter is 0.
*/
void DecRef(char *b, int n)
{
  TFileBlock *pFileBlock;

  ASSERT(b != NULL);
  ASSERT(n > 0);

  pFileBlock = (TFileBlock *)b - 1;
  ASSERT(n <= pFileBlock->nRef);
  pFileBlock->nRef -= n;
  if (pFileBlock->nRef == 0)
  {
    REMOVE_ENTRY_LIST(&pFileBlock->link);
    s_free(pFileBlock);
  }
}

/* ************************************************************************
   Function: DisposeBlock
   Description:
     Usually a block should be disposed by DecRef().
     This function is only for emergeny purposes or
     when the whole block is necessary to be disposed
     irrelevantly to the collected references.
*/
void DisposeBlock(char *b)
{
  TFileBlock *pFileBlock;

  ASSERT(b != NULL);

  pFileBlock = (TFileBlock *)b - 1;
  REMOVE_ENTRY_LIST(&pFileBlock->link);
  s_free(pFileBlock);
}

/* ************************************************************************
   Function: DisposeBlockList
   Description:
     Disposes a whole block list. Usually used when a file
     is to be closed and disposed from memory.
*/
void DisposeBlockList(TListRoot *blist)
{
  TFileBlock *pFileBlock;

  ASSERT(blist != NULL);

  while	(!IS_LIST_EMPTY(blist))
  {
    pFileBlock = (TFileBlock *)REMOVE_TAIL_LIST(blist);
    s_free(pFileBlock);
  }
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

