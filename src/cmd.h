/*

File: cmd.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 29th October, 1998
Descrition:
  Keyboard to Command map supporting functions.

*/

#ifndef CMD_H
#define CMD_H

#include "clist.h"
#include "l1opt.h"
#include "disp.h"

typedef struct CmdDesc
{
  int nCode;  /* Unique command code -- an cmXXX constant */
  const char *sName;  /* Command title */
  void (*pFunc)(void *pCtx);  /* Command function */
  const char *sHelpPage;  /* Help page title in the info file (context help) */
  const char *sCtxMenu;  /* Item in the context help stack menu */
} TCmdDesc;

/* END_OF_CMD_LIST_CODE is choosen to be used for testing as well */
#define END_OF_CMD_LIST_CODE   (MAX_CMD_CODE + 1)
#define EndOfCmdList(item) ((item).nCode == END_OF_CMD_LIST_CODE)

typedef struct KeySequence
{
  int nCode;  /* Command ID to be invoked */
  int nNumber;  /* How match keys are in this particular key sequence */
  DWORD dwKeySeq[MAX_KEY_SEQ];  /* The keys */
} TKeySequence;

/* END_OF_KEY_LIST_CODE is choosen to be used for testing as well */
#define END_OF_KEY_LIST_CODE   (MAX_CMD_CODE + 2)
#define EndOfKeyList(item) ((item).nCode == END_OF_KEY_LIST_CODE)

#define DEF_KEY1(a) 1, {a}
#define DEF_KEY2(a, b) 2, {a, b}

typedef struct KeySet
{
  TKeySequence *pKeySet;
  int nNumberOfItems;
  BOOLEAN bChanged;
} TKeySet;

void ClearKeySequence(TKeySequence *pKeySeq);
void AddKey(TKeySequence *pKeySeq, DWORD dwKey);
int ChkKeySequence(const TKeySequence *pKeySet, const TKeySequence *pKeySeq);
BOOLEAN ExecuteCommand(const TCmdDesc *pCommands, int nCode,
  int nNumberOfElements, void *pCtx);
int SortCommands(TCmdDesc *pCommands);
int CountKeySequence(TKeySequence *pKeySeq);
#ifdef _DEBUG
void DumpKeySet(const TKeySequence *pKeySet, dispc_t *disp);
#endif

#endif /* ifndef CMD_H */

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

