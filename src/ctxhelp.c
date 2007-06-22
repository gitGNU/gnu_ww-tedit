/*

File: ctxhelp.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 15th October, 2001
Descrition:
  Context sensitive help maintanance functions.

*/

#include "global.h"
#include "menu.h"
#include "disp.h"
#include "palette.h"
#include "infordr.h"
#include "helpcmd.h"
#include "ctxhelp.h"

char *pCtxSep = "separator";  /* use this to put a separator in the contexthlp menu */

/* ************************************************************************
   Function: HelpPushContext
   Description:
     Pushes a help page in the help context stack.
*/
void HelpPushContext(TContextHelp *pCtx, const char *psHelpItem,
  const char *psMenuItem, const char *psInfoFile)
{
  ASSERT(pCtx->nTop >= 0);
  ASSERT(pCtx->nTop < MAX_CTXHLP_DEPTH);

  pCtx->stContext[pCtx->nTop].psHelpPage = psHelpItem;
  pCtx->stContext[pCtx->nTop].psInfoFile = psInfoFile;
  pCtx->stContext[pCtx->nTop].psMenuItem = psMenuItem;
  ++pCtx->nTop;
  return;
}

/* ************************************************************************
   Function: HelpPopContext
   Description:
     Pops elements from the context help stack
*/
void HelpPopContext(TContextHelp *pCtx, int nLevels)
{
  ASSERT(nLevels <= pCtx->nTop);

  pCtx->nTop -= nLevels;
}

/* ************************************************************************
   Function: ShowContextHelp
   Description:
     Shows menu with the current context help stack
     and invokes the help viewer with the selected item.
*/
void ShowContextHelp(TContextHelp *pCtx)
{
  TMenuItem stMenuItems[MAX_CTXHLP_DEPTH];
  TMenuItem *stItems[MAX_CTXHLP_DEPTH];
  TMenu stMenu;
  int x;
  int y;
  int i;
  int j;
  const char *psInfoFile;

  if (pCtx->nTop == 0)
    return;

  memset(stMenuItems, 0, sizeof(stMenuItems));
  memset(stItems, 0, sizeof(stItems));
  memset(&stMenu, 0, sizeof(stMenu));

  for (j = 0, i = pCtx->nTop - 1; i >= 0; --i, ++j)
  {
    if (pCtx->stContext[i].psMenuItem == pCtxSep)
      stItems[j] = &itSep;
    else
    {
      stMenuItems[j].Prompt = pCtx->stContext[i].psMenuItem;
      stMenuItems[j].Command = i;
      stItems[j] = &stMenuItems[j];
    }
  }

  stMenu.ItemsNumber = pCtx->nTop;
  stMenu.Items = (TMenuItem *(*)[])stItems;
  stMenu.nPalette = coCtxHlpMenu;
  disp_cursor_get_xy(0, &x, &y);
  i = Menu(&stMenu, x, y, 0, 0);
  if (i <= 0)
    return;

  psInfoFile = pCtx->stContext[i].psInfoFile;
  if (psInfoFile == NULL)
    psInfoFile = sHelpFile;
  NavigateInfoPage(psInfoFile, pCtx->stContext[i].psHelpPage, FALSE);
}

/* ************************************************************************
   Function: InitContextHelp
   Description:
     Initial setup of a context help structure.
*/
void InitContextHelp(TContextHelp *pCtx)
{
}

/* ************************************************************************
   Function: DoneContextHelp
   Description:
     Done, final, it's clear!
*/
void DoneContextHelp(TContextHelp *pCtx)
{
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

