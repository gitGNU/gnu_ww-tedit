/*

File: cmd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 29th October, 1998
Descrition:
  Keyboard to Command map supporting functions.

*/

#include "global.h"
#include "clist.h"
#include "memory.h"
#include "l1opt.h"
#include "keyset.h"
#include "disp.h"
#include "cmd.h"

#ifdef _DEBUG
/* ************************************************************************
   Function: DumpKeySeq
   Description:
     Dumps a key sequence
*/
void DumpKeySeq(FILE *f, dispc_t *disp, const TKeySequence *pKeyItem)
{
  int i;
  char sKeyName[35];

  fprintf(f, ":");
  for (i = 0; i < pKeyItem->nNumber; i++)
  {
    disp_get_key_name(disp, pKeyItem->dwKeySeq[i], sKeyName, sizeof(sKeyName));
    fprintf(f, "%s (%#x);", sKeyName, (int)pKeyItem->dwKeySeq[i]);
  }
  fprintf(f, "\n");
}

/* ************************************************************************
   Function: DumpKeySet
   Description:
     Dumps the key set in a file. This function is with
     control purposes.
*/
void DumpKeySet(const TKeySequence *pKeySet, dispc_t *disp)
{
  FILE *f;
  const TKeySequence *pKeyItem;

  ASSERT((f = fopen("keyset.txt", "wt")) != NULL);

  for (pKeyItem = pKeySet; !EndOfKeyList(*pKeyItem); pKeyItem++)
    DumpKeySeq(f, disp, pKeyItem);

  fclose(f);
}
#endif

/* ************************************************************************
   Function: ClearKeySequence
   Description:
     Clears a key sequence
*/
void ClearKeySequence(TKeySequence *pKeySeq)
{
  ASSERT(pKeySeq != NULL);

  pKeySeq->nNumber = 0;
}

/* ************************************************************************
   Function: AddKey
   Description:
     Adds a key	to a key sequence
*/
void AddKey(TKeySequence *pKeySeq, DWORD dwKey)
{
  ASSERT(pKeySeq != NULL);
  ASSERT(pKeySeq->nNumber < MAX_KEY_SEQ - 1);

  pKeySeq->dwKeySeq[pKeySeq->nNumber++] = NO_ASC(dwKey);
}

/* ************************************************************************
   Function: KeySeqMatch
   Description:
     Returns the number of the matching keys of pKeySeq2 in pKeySeq1
*/
static int KeySeqMatch(const TKeySequence *pKeySeq1, const TKeySequence *pKeySeq2)
{
  int i;
  DWORD dwKey1, dwKey2;
  int n;

  ASSERT(pKeySeq1 != NULL);
  ASSERT(pKeySeq2 != NULL);
  ASSERT(pKeySeq1->nNumber > 0);
  ASSERT(pKeySeq1->nNumber < MAX_KEY_SEQ);
  ASSERT(pKeySeq2->nNumber > 0);
  ASSERT(pKeySeq2->nNumber < MAX_KEY_SEQ);

  n = pKeySeq1->nNumber < pKeySeq2->nNumber ? pKeySeq1->nNumber : pKeySeq2->nNumber;

  for (i = 0; i < n; ++i)
  {
    dwKey1 = pKeySeq1->dwKeySeq[i];
    dwKey2 = pKeySeq2->dwKeySeq[i];
    if (!bStrictCheck)
      if (i > 0)
      {
	dwKey1 = NO_SH_STATE2(kbCtrl + kbShift, dwKey1);
        dwKey2 = NO_SH_STATE2(kbCtrl + kbShift, dwKey2);
      }
    if (dwKey1 != dwKey2)
      break;
  }

  return (i);
}

/* ************************************************************************
   Function: ChkKeySequence
   Description:
     Check whether a key sequence is valid
   Returns:
     -1 -- there's still more keys to be collected
     -2 -- this is not a valid key sequence
     n -- command code number that matches the key sequence
*/
int ChkKeySequence(const TKeySequence *pKeySet, const TKeySequence *pKeySeq)
{
  int n;
  int iMaxMatch;
  const TKeySequence *pKeyItem;

  ASSERT(pKeySeq != NULL);
  ASSERT(pKeySet != NULL);

  #ifdef _DEBUG
  {
    const TKeySequence *pKeyItem;

    /*
      How to check whether pKeySet doesn't point at a wrong plase?
      1. To check whether there's no items
      2. To check whether there are no more than MAX_KEYS item.
    */
    ASSERT(!EndOfKeyList(pKeySet[0]));

    for (pKeyItem = pKeySet; !EndOfKeyList(*pKeyItem); pKeyItem++)
      ASSERT(pKeyItem - pKeySet <= MAX_KEYS);
  }
  #endif

  /* Check for partial match of the sequence up to here */
  iMaxMatch = 0;
  for (pKeyItem = pKeySet; !EndOfKeyList(*pKeyItem); pKeyItem++)
  {
    n = KeySeqMatch(pKeyItem, pKeySeq);
    if (n > iMaxMatch)
      iMaxMatch = n;
    if (iMaxMatch == pKeySeq->nNumber)  /* Key seq search was exousted */
    {
      if (pKeyItem->nNumber == n)  /* Complete match? */
        return (pKeyItem->nCode);
      else
        return (-1);  /* Full match not found */
    }
  }

  ASSERT(pKeyItem - pKeySet > 0);  /* Command set should be not empty */

  return (-2);  /* There's no command for such a key sequence */
}

/* ************************************************************************
   Function: CompareInt
   Description:
     Call-back function to be passed to qsort and bsearch.
*/
static int CompareCmdID(const void *arg1, const void *arg2)
{
  const TCmdDesc *i1 = (const TCmdDesc *)arg1;
  const TCmdDesc *i2 = (const TCmdDesc *)arg2;

  ASSERT(i1->nCode > 0);
  ASSERT(i2->nCode > 0);
  ASSERT(i1->nCode < MAX_CMD_CODE);
  ASSERT(i2->nCode < MAX_CMD_CODE);

  return (i1->nCode - i2->nCode);
}

/* ************************************************************************
   Function: ExecuteCommand
   Description:
     Executes a command	selected by a specific code
*/
BOOLEAN ExecuteCommand(const TCmdDesc *pCommands, int nCode,
  int nNumberOfElements, void *pCtx)
{
  const TCmdDesc *pCmdItem;
  TCmdDesc CmdItemKey;

  ASSERT(pCommands != NULL);
  ASSERT(!EndOfCmdList(pCommands[0]));
  ASSERT(nCode != 0);
  ASSERT(nNumberOfElements > 0);

  #ifdef _DEBUG
  {
    for (pCmdItem = pCommands; !EndOfCmdList(*pCmdItem); pCmdItem++)
    {
      ASSERT(pCmdItem - pCommands <= nNumberOfElements);  /* Missing END_OF_LIST */
    }
  }
  #endif

  ASSERT(nCode > 0);
  ASSERT(nCode < MAX_CMD_CODE);
  CmdItemKey.nCode = nCode;
  pCmdItem = bsearch(&CmdItemKey, pCommands, nNumberOfElements,
    sizeof(TCmdDesc), CompareCmdID);
  if (pCmdItem == NULL)
    return FALSE;
  ASSERT(pCmdItem != NULL);
  (*pCmdItem->pFunc)(pCtx);
  return TRUE;
}

/* ************************************************************************
   Function: SortCommands
   Description:
     Sorts the pCommands array by nCodeID.
*/
int SortCommands(TCmdDesc *pCommands)
{
  TCmdDesc *pCmdItem;
  int nNumberOfCmdElements;

  ASSERT(!EndOfCmdList(pCommands[0]));

  /* Count the number of command descriptions */
  for (pCmdItem = pCommands; !EndOfCmdList(*pCmdItem); pCmdItem++)
  {
    ASSERT(pCmdItem - pCommands <= 200);  /* Missing END_OF_LIST */
  }

  nNumberOfCmdElements = (int)(pCmdItem - pCommands);
  qsort(pCommands, nNumberOfCmdElements, sizeof(TCmdDesc), CompareCmdID);
  return (nNumberOfCmdElements);
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

