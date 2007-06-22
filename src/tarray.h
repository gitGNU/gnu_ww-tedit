/*

File: tarray.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Dynamic size type independent array macroses.

*/

#ifndef _TARRAY_H
#define _TARRAY_H

typedef struct Array
{
  #ifdef _DEBUG
  BYTE MagicByte;
  #define ARRAY_MAGIC 0x51
  #endif
  int nCount;
  int nDelta;
  int nSize;
  BOOLEAN bStatus;
  void *dummy;  /* alignment */
} _TArray;

#ifdef _DEBUG
#define VALID_PARRAY(pArray)  ((pArray) != NULL && ((_TArray *)(pArray) - 1)->MagicByte == ARRAY_MAGIC)
#else
#define VALID_PARRAY(pArray)  (1)
#endif

#ifdef _DEBUG
#define SET_MAGIC_BYTE(pArray, v)  pArray->MagicByte = v
#else
#define SET_MAGIC_BYTE(pArray, v)
#endif

#define TArrayDumpInfo(x)\
  {\
  _TArray *p;\
  \
  p = ((_TArray *)x) - 1;\
  printf("Index: %p\n", x);\
  printf("TArray: %p\n", p);\
  printf("Item size: %d\n", sizeof(*x));\
  printf("nCount: %u\n", p->nCount);\
  printf("nDelta: %u\n", p->nDelta);\
  printf("nSize: %u\n", p->nSize);\
  }\

#define _TArrayCount(x) (((_TArray *)x - 1)->nCount)
#define TArraySetCount(x, v) (((_TArray *)x - 1)->nCount = v)
#define TArrayStatus(x) (((_TArray *)x - 1)->bStatus)
#define TArrayClearStatus(x)  (((_TArray *)x - 1)->bStatus = TRUE)

/* ************************************************************************
   Function: TArray
   Description:
     Declares a dynamic array of type 'x'.
*/
#define TArray(x)  x*

/* ************************************************************************
   Function: TArrayInit
   Description:
     Initial setup of a TArray type declared variable 'x'.
   On entry:
     x - a variable declared by TArray macro.
     _nInitial - initial size.
     _nDelta - grow up size of the array.
   On exit:
     x - points to an array
     and TArray internal fields are initialized.

   Initial size is calculated to be denominated to _nDelta.

   On exit check whether x != NULL for success.
*/
#define TArrayInit(x, _nInitial, _nDelta)\
  {\
  _TArray *p;\
  int _nInitial2;\
  \
  x = NULL;\
  _nInitial2 = _nInitial + _nDelta - _nInitial % _nDelta;\
  p = (_TArray *)alloc(sizeof(_TArray) + sizeof(*x) * _nInitial2);\
  if (p != NULL)\
  {\
    SET_MAGIC_BYTE(p, ARRAY_MAGIC);\
    p->nCount = 0;\
    p->nDelta = _nDelta;\
    ASSERT(_nDelta > 0);\
    p->nSize = _nInitial2;\
    p->bStatus = TRUE;\
    \
    x = (void *)(p + 1);\
  }\
  }

/* ************************************************************************
   Function: TArrayDispose
   Description:
     Disposes an TArray	block.
*/
#define TArrayDispose(x)\
  {\
  _TArray *p;\
  \
  ASSERT(x != NULL);\
  \
  p = ((_TArray *)x) - 1;\
  SET_MAGIC_BYTE(p, 0);\
  s_free(p);\
  x = NULL;\
  }

/* ************************************************************************
   Function: TArrayInsert
   Description:
     Inserts at certain position by moving all the elements to make space.
   On entry:
     x - _TArray
     nPos - Position to insert to.
     y - an element to insert (address of an element)

   Check TArrayStatus()	as an indication the operation succeeded!
*/
#define TArrayInsert(x,	nPos,  y)\
  {\
  _TArray *p;\
  int nNewSize;\
  \
  ASSERT(x != NULL);\
  \
  p = ((_TArray *)x) - 1;\
  if (p->nCount + 1 > p->nSize)\
  {\
    nNewSize = p->nSize + p->nDelta;\
    if (!SafeRealloc(\
      (void **)&p,\
      sizeof(*x) * p->nSize + sizeof(_TArray),\
      sizeof(*x) * nNewSize + sizeof(_TArray)))\
    {\
      p->bStatus = FALSE;\
    }\
    else\
    {\
      p->nSize = nNewSize;\
      x = (void *)(p + 1);\
    }\
  }\
  if (p->bStatus)\
  {\
    if (nPos < p->nCount)\
      memmove(x + (nPos) + 1, x + (nPos), (p->nCount - (nPos)) * sizeof(*x));\
    memcpy(x + (nPos), &y, sizeof(*x));\
    ++p->nCount;\
  }\
  }

/* ************************************************************************
   Function: TArrayInsertGroup
   Description:
     Insert at certain position by moving all the elements to make space.
   On entry:
     x - _TArray
     nPos - Position to insert to.
     y - pointer to group of elements to insert.
     nCount - number of elements to insert at nPos.

   nNewSize is calculated to be always denominated to p->nDelta.

   Check TArrayStatus()	as an indication the operation succeeded!
*/
#define TArrayInsertGroup(x, nPos, y, _nCount)\
  {\
  _TArray *p;\
  int nNewSize;\
  \
  ASSERT(x != NULL);\
  \
  p = ((_TArray *)x) - 1;\
  if (p->nCount + (_nCount) > p->nSize)\
  {\
    nNewSize = p->nSize + ((_nCount) / p->nDelta + 1) * p->nDelta;\
    if (!SafeRealloc(\
      (void **)&p,\
      sizeof(*x) * p->nSize + sizeof(_TArray),\
      sizeof(*x) * nNewSize + sizeof(_TArray)))\
    {\
      p->bStatus = FALSE;\
    }\
    else\
    {\
      p->nSize = nNewSize;\
      x = (void *)(p + 1);\
    }\
  }\
  if (p->bStatus)\
  {\
    if (nPos < p->nCount)\
      memmove(x + (nPos) + (_nCount), x + (nPos), (p->nCount - (nPos)) * sizeof(*x));\
    memcpy(x + (nPos), y, (_nCount) * sizeof(*x));\
    p->nCount += (_nCount);\
  }\
  }

/* ************************************************************************
   Function: TArrayAdd
   Description:
     Adds an element at the end of the array.
   On entry:
     x - _TArray
     y - an element to add (address of an element)

   Check TArrayStatus()	as an indication the operation succeeded!
*/
#define TArrayAdd(x, y)\
  {\
  _TArray *p;\
  int nNewSize;\
  \
  p = ((_TArray *)x) - 1;\
  if (p->nCount + 1 > p->nSize)\
  {\
    nNewSize = p->nSize + p->nDelta;\
    if (!SafeRealloc(\
      (void **)&p,\
      sizeof(*x) * p->nSize + sizeof(_TArray),\
      sizeof(*x) * nNewSize + sizeof(_TArray)))\
    {\
      p->bStatus = FALSE;\
    }\
    else\
    {\
      p->nSize = nNewSize;\
      x = (void *)(p + 1);\
    }\
  }\
  if (p->bStatus)\
  {\
    memcpy(x + (p->nCount), &y, sizeof(*x));\
    ++p->nCount;\
  }\
  }

/* ************************************************************************
   Function: TArrayDeleteGroup
   Description:
     Deletes a group of elements.
   On entry:
     x - _TArray
     nPos - Position to delete from.
     nCount - number of elements to delete at nPos.
*/
#define TArrayDeleteGroup(x, nPos, _nCount)\
  {\
  _TArray *p;\
  \
  p = ((_TArray *)x) - 1;\
  if (nPos + _nCount < p->nCount)\
    memmove(x + (nPos), x + (nPos) + (_nCount), (p->nCount - (nPos) - (_nCount)) * sizeof(*x));\
  p->nCount -= _nCount;\
  }

#endif /* _TARRAY_H */

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

