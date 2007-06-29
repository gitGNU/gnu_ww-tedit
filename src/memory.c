/*

File: memory.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Safety pool support.

*/

#include "global.h"
#include "heapg.h"
#include "l1opt.h"
#include "memory.h"

void *pSafetyPool;
BOOLEAN bNoMemory;

#ifdef HEAP_DBG
BOOLEAN bReturnNULL;
BOOLEAN bTraceMemory;
#endif

/* ************************************************************************
   Function: s_xmalloc
   Description:
     Allocates space and activates the safety pool if no memory.
*/
#ifdef HEAP_DBG
void *_s_xmalloc(int size, const char *name, int line)
{
  void *p;

__try__alloc:
  p = _dbg__xmalloc(size, name, line);
  if (bTraceMemory)
    TRACE3(":%s:%d-%d bytes\n", name, line, size);
  if (p == NULL)
  {
    if (pSafetyPool)
    {
      xfree(pSafetyPool);
      pSafetyPool = NULL;
      goto __try__alloc;
    }
    else
    {
      printf("Safety pool exhausted!\n");
      abort();
    }
  }
  return (p);
}

/* ************************************************************************
   Function: alloc
   Description:
     Allocates space if no Safety Pool activated.
*/
void *_alloc(int size, const char *name, int line)
{
  void *p;

  if (bReturnNULL)  /* Simulate	exhausted memory */
  {
    bNoMemory = TRUE;
    return (NULL);
  }
  if (pSafetyPool)
  {
    if (bTraceMemory)
      TRACE3(":%s:%d-%d bytes\n", name, line, size);
    p = _dbg__xmalloc(size, name, line);
    if (!p)
      bNoMemory = TRUE;  /* Indicate that the memory was exhausted */
    return (p);
  }
  else
    return (NULL);
}

/* ************************************************************************
   Function: s_free
   Description:
     xfree wrap function. Tries to keep the Safety Pool.
     This function should be called for	s_alloc and alloc.
*/
void _s_free(void *p, const char *name, int line)
{
  _dbg_xfree(p, name, line);
  if (!pSafetyPool)
    pSafetyPool = _xmalloc(SAFETY_POOL);
}
#else
void *_s_xmalloc(int size)
{
  void *p;

  __try__alloc:
  p = malloc(size);
  if (p == NULL)
  {
    if (pSafetyPool)
    {
      free(pSafetyPool);
      pSafetyPool = NULL;
      goto __try__alloc;
    }
    else
    {
      printf("Safety pool exhausted!\n");
      abort();
    }
  }
  return (p);
}

/* ************************************************************************
   Function: alloc
   Description:
     Allocates space if no Safety Pool activated.
*/
void *_alloc(int size)
{
  void *p;

  if (pSafetyPool)
  {
    p = malloc(size);
    if (!p)
      bNoMemory = TRUE;  /* Indicate that the memory was exhausted */
    return (p);
  }
  else
  {
    bNoMemory = TRUE;
    return (NULL);
  }
}

/* ************************************************************************
   Function: s_free
   Description:
     xfree wrap function. Tries to keep the Safety Pool.
     This function should be called for	s_alloc and alloc.
*/
void _s_free(void *p)
{
  free(p);
  if (!pSafetyPool)
    pSafetyPool = malloc(SAFETY_POOL);
}
#endif

/* ************************************************************************
   Function: InitSafatyPool
   Description:
     Allocates memory for the safety pool.
*/
void InitSafetyPool(void)
{
  if (!(pSafetyPool = _xmalloc(SAFETY_POOL)))
  {
    printf("w32: No memory to run the application\n");
    abort();
  }
  #ifdef HEAP_DBG
  bReturnNULL = FALSE;
  bTraceMemory = FALSE;
  #endif
}

/* ************************************************************************
   Function: DisposeSafetyPool
   Description:
*/
void DisposeSafetyPool(void)
{
  if (pSafetyPool)
    xfree(pSafetyPool);
}

/* ************************************************************************
   Function: SafeRealloc
   Description:
     Reallocates a block. In contrast with standard realloc() doesn't
     remove the old block in case of failure.
*/
BOOLEAN SafeRealloc(void **pBlock, int nSize, int nNewSize)
{
  void *pNewBlock;
  int nToCopy;

  ASSERT(pBlock	!= NULL);
  ASSERT(*pBlock != NULL);
  ASSERT(nSize != 0);
  ASSERT(nNewSize != 0);
  ASSERT(nSize != nNewSize);  /* This may prevent an error */

  #ifdef HEAP_DBG
  if (bReturnNULL)  /* Simulate exhausted memory */
  {
    bNoMemory = TRUE;
    return (FALSE);
  }
  #endif

  pNewBlock = xmalloc(nNewSize);
  if (pNewBlock	== NULL)
  {
    bNoMemory = TRUE;
    return (FALSE);
  }

  nToCopy = nSize;
  if (nNewSize < nSize)
    nToCopy = nNewSize;

  memcpy(pNewBlock, *pBlock, nToCopy);
  xfree(*pBlock);
  *pBlock = pNewBlock;
  return (TRUE);
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

