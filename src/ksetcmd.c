/*

File: ksetcmd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 30th June, 1999
Descrition:
  Commands for manipulation of key set.

*/

#include "global.h"
#include "tarray.h"
#include "keyset.h"
#include "defs.h"
#include "disp.h"
#include "l2disp.h"
#include "memory.h"
#include "palette.h"
#include "umenu.h"
#include "menudat.h"
#include "cmdc.h"
#include "wrkspace.h"
#include "ksetcmd.h"


int CmdCodeIndex[1024];

static TKeySequence InvokeHlp[] =
{
  {1, DEF_KEY1(KEY(0, kbF1))},
  {END_OF_KEY_LIST_CODE}  /* LastItem */
};

/* ************************************************************************
   Function: Help_DlgProc
   Description:
     Call-back function invoked by UMenu() when a key
     from InvokeHlp[] keyset is pressed.
*/
static int Help_DlgProc(int nCurItem, int nCmd)
{
  int x;
  int y;

  switch (nCmd)
  {
    case 1:
      /* TODO: disp */
      //disp_cursor_get_xy(0, &x, &y);
      x = 0;
      y = 0;
      InvokeCommandHelp(CmdCodeIndex[nCurItem], x, y);
      break;

    default:
      ASSERT(0);
  }

  return 0;
}

/* ************************************************************************
   Function: DisplayKeys
   Description:
     Displays current key set.
*/
void DisplayKeys(TCmdDesc *pCommands, TKeySequence *pKeySet, dispc_t *disp)
{
  void *pStatLn;
  TArray(char *) pLineIndex;
  char *pDest;
  char sBuf[80];
  char sKeyName[35];
  int i;
  BOOLEAN bHasKey;
  const TKeySequence *pKeyItem;
  const TCmdDesc *pCmdItem;
  int nDummy;
  char sDummy[80];

  TArrayInit(pLineIndex, 50, 25);
  if (!TArrayStatus(pLineIndex))
    return;

  /*
  Walk through Commands[] and for each of the commands
  serch in KeySet for assigned keys.
  */
  for (pCmdItem = pCommands; !EndOfCmdList(*pCmdItem); ++pCmdItem)
  {
    bHasKey = FALSE;
    for (pKeyItem = pKeySet; !EndOfKeyList(*pKeyItem); ++pKeyItem)
    {
      if (pCmdItem->nCode == pKeyItem->nCode)
      {
        bHasKey = TRUE;
	sprintf(sBuf, "%-25s: ", pCmdItem->sName);
        for (i = 0; i < pKeyItem->nNumber; ++i)
        {
          disp_get_key_name(disp, pKeyItem->dwKeySeq[i], sKeyName, sizeof(sKeyName));
          ASSERT(strlen(sKeyName) < 35);  /* otherwise we have overflow */
	  strcat(sBuf, sKeyName);
        }
        pDest = s_alloc(strlen(sBuf) + 1);
        strcpy(pDest, sBuf);
        TArrayAdd(pLineIndex, pDest);
        if (!TArrayStatus(pLineIndex))
          goto _dispose_array;
        ASSERT(_TArrayCount(pLineIndex) < 1024);
        CmdCodeIndex[_TArrayCount(pLineIndex) - 1] = pCmdItem->nCode;
      }
    }
    if (!bHasKey)
    {
      sprintf(sBuf, "%-25s: ", pCmdItem->sName);
      pDest = s_alloc(strlen(sBuf) + 1);
      strcpy(pDest, sBuf);
      TArrayAdd(pLineIndex, pDest);
      if (!TArrayStatus(pLineIndex))
        goto _dispose_array;
      ASSERT(_TArrayCount(pLineIndex) < 1024);
      CmdCodeIndex[_TArrayCount(pLineIndex) - 1] = pCmdItem->nCode;
    }
  }

  pDest = NULL;  /* Terminal symbol */
  TArrayAdd(pLineIndex, pDest);
  if (!TArrayStatus(pLineIndex))
    goto _dispose_array;

  pStatLn = SaveStatusLine(disp);
  DisplayStatusStr2(sStatHelp, coStatusTxt, coStatusShortCut, disp);
  nDummy = 0;
  UMenu(0, 0, 1, disp_wnd_get_height(0) - 3, disp_wnd_get_width(disp) - 3,
        "Keys", (const char **)pLineIndex, sDummy, &nDummy,
        InvokeHlp, Help_DlgProc, coHelpList, disp);
  RestoreStatusLine(pStatLn, disp);

_dispose_array:
  for (i = 0; i < _TArrayCount(pLineIndex) - 1; ++i)
    s_free(pLineIndex[i]);

  TArrayDispose(pLineIndex);
}

/* ************************************************************************
   Function: CmdHelpKbd
   Description:
     Displays current key set.
*/
void CmdHelpKbd(void *pCtx)
{
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));
  DisplayKeys(Commands, KeySet.pKeySet, disp);
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

