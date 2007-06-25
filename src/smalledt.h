/*

File: smalledt.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 1st May, 2000
Descrition:
  Small editor/viewer.
  One example use if as viewer for FindInFiles results, or as editor for
  comment headers.

*/

#ifndef SMALLEDT_H
#define SMALLEDT_H

#include "cmd.h"
#include "file.h"
#include "fview.h"

typedef struct _SEInterface
{
  #ifdef _DEBUG
  BYTE MagicByte;
  #define SE_INTERFACE_MAGIC  0x5a
  #endif
  BOOLEAN (*CmdEnter)(void *pCtx);
  BOOLEAN (*CmdDelete)(void *pCtx);
  void *pCtx;
} TSEInterface;

#ifdef _DEBUG
#define VALID_PSE_INTERFACE(pSEInterface) ((pSEInterface) != NULL && (pSEInterface)->MagicByte == SE_INTERFACE_MAGIC)
#else
#define VALID_PSE_INTERFACE(pSEInterface)  (1)
#endif

void ComposeSmallEditKeySet(TKeySet *pMainKeySet);
void SmallEdtSetHandlers(TFileView *pFileView);
void SmallEdit(TFile *_pFile, TSEInterface *pstCallBackInterface,
  BOOLEAN bDisplayOnly);

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

