/*

File: searcmd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 24th October, 1999
Descrition:
  Commands concerning text search and replace.

*/

#include "global.h"
#include "wrkspace.h"
#include "disp.h"
#include "palette.h"
#include "l1def.h"
#include "l2disp.h"
#include "enterln.h"
#include "defs.h"
#include "nav.h"
#include "search.h"
#include "searchf.h"
#include "cmdc.h"
#include "searcmd.h"
#include "bookmcmd.h"
#include "doctype.h"
#include "wrkspace.h"

/* ************************************************************************
   Function: CmdIncrementalSearch
   Description:
*/
void CmdEditIncrementalSearch(void *pCtx)
{
  disp_wnd_param_t wnd_param;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  disp_wnd_get_param(disp, &wnd_param);
  _CmdIncrementalSearch(CMDC_PFILE(pCtx), 1, wnd_param.width, &stSearchContext);
}

/* ************************************************************************
   Function: CmdIncrementalSearchBack
   Description:
*/
void CmdEditIncrementalSearchBack(void *pCtx)
{
  disp_wnd_param_t wnd_param;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  disp_wnd_get_param(disp, &wnd_param);
  _CmdIncrementalSearch(CMDC_PFILE(pCtx), -1, wnd_param.width, &stSearchContext);
}

/* ************************************************************************
   Function: GetSelected
   Description:
*/
static BOOLEAN GetSelected(const TFile *pFile)
{
  TLine *pLine;
  int nGet;
  int nLen;
  char sSearch[MAX_SEARCH_STR];

  ASSERT(VALID_PFILE(pFile));

  if (!pFile->bBlock)
    return FALSE;

  if ((pFile->blockattr & COLUMN_BLOCK) != 0)
    return FALSE;

  if (pFile->nEndLine - pFile->nStartLine != 0)
    return FALSE;  /* No search of multiline selections */

  pLine = GetLine(pFile, pFile->nStartLine);
  nLen = pFile->nEndPos - pFile->nStartPos + 1;
  nGet = nLen < MAX_SEARCH_STR ? nLen : MAX_SEARCH_STR;
  strncpy(sSearch, &pLine->pLine[pFile->nStartPos], nGet);
  sSearch[nGet] = '\0';
  ClearSearchContext(&stSearchContext);
  stSearchContext.bRegularExpr = FALSE;
  stSearchContext.bCaseSensitive = FALSE;
  NewSearch(&stSearchContext, sSearch, 0);
  return TRUE;
}

/* ************************************************************************
   Function: GetWord
   Description:
     Establishes the word under the cursor as current search pattern.
*/
static BOOLEAN GetWord(const TFile *pFile)
{
  char sSearch[MAX_SEARCH_STR];

  GetWordUnderCursor(pFile, sSearch, sizeof(sSearch) - 1);
  if (sSearch[0] == '\0')
    return FALSE;

  ClearSearchContext(&stSearchContext);
  stSearchContext.bRegularExpr = FALSE;
  stSearchContext.bCaseSensitive = FALSE;
  NewSearch(&stSearchContext, sSearch, 0);
  return TRUE;
}

/* ************************************************************************
   Function: CmdEditFindSelected
   Description:
*/
void CmdEditFindSelected(void *pCtx)
{
  TFile *pFile;
  disp_wnd_param_t wnd_param;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  pFile = CMDC_PFILE(pCtx);
  if (!GetSelected(pFile))
    if (!GetWord(pFile))
      return;

  disp_wnd_get_param(disp, &wnd_param);
  Find(pFile, 1, wnd_param.width, &stSearchContext);
  AddHistoryLine(pFindHistory, stSearchContext.sSearch);
}

/* ************************************************************************
   Function: CmdEditFindSelectedBack
   Description:
*/
void CmdEditFindSelectedBack(void *pCtx)
{
  TFile *pFile;
  disp_wnd_param_t wnd_param;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  pFile = CMDC_PFILE(pCtx);
  if (!GetSelected(pFile))
    if (!GetWord(pFile))
      return;

  if (pFile->bBlock)
    GotoPosRow(pFile, pFile->nStartPos, pFile->nRow);
  disp_wnd_get_param(disp, &wnd_param);
  Find(pFile, -1, wnd_param.width, &stSearchContext);
  AddHistoryLine(pFindHistory, stSearchContext.sSearch);
}

/* ************************************************************************
   Function: CmdEditFindNext
   Description:
*/
void CmdEditFindNext(void *pCtx)
{
  disp_wnd_param_t wnd_param;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  disp_wnd_get_param(disp, &wnd_param);
  Find(CMDC_PFILE(pCtx), 1, wnd_param.width, &stSearchContext);
}

/* ************************************************************************
   Function: CmdEditFindBack
   Description:
*/
void CmdEditFindBack(void *pCtx)
{
  disp_wnd_param_t wnd_param;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  disp_wnd_get_param(disp, &wnd_param);
  Find(CMDC_PFILE(pCtx), -1, wnd_param.width, &stSearchContext);
}

/* ************************************************************************
   Function: PrepareInvalidSwitchMsg
   Description:
     Prepares the "Invalid switch message"
*/
static void PrepareInvalidSwitchMsg(char *buf, const char *sSwitch, const char *sText)
{
  sprintf(buf, sInvalidSwitch, sText, sSwitch);
}

/* ************************************************************************
   Function: ParseFindInput
   Description:
     Parses Find&Replace prompt:
     format:
     [options] [m|s]/search_pattern/replace_pattern/[i|g|m]
     options are:
       -r[+|-] match regular expressions, default is -
       -i[+|-] ignore case, default is +
       -b[+|-] search backward, default is -
       -n[+|-] replace with no prompt, default is -
       -mN set multiple line buffer of N lines
     m - match, can be skipped if separator is '/'
     s - replace
     options after the pattern
     i - same as -i+ (maintained only for compatibility with perl)
     m - treat string as multiple lines (\n is multiline separator)
     Furthermore:
     --the separators can be omitted altogether then search operation
     is assumed
     --m can be omitted, then seach operation is assumed
     --m (the options at the end) can be omitted, the string
     is always autodetected as multipleline if '\n' is present in the
     search_pattern or replace_pattern
     --s can be omitted if replace pattern is supplied

     Options by examples:
     the string: [1]
     -ioresult
     will emit an error that the option has an invalid switch "oreslt"

     the string:
     -i oresult test
     will search for the string "oresult test"

     the string
     -i -b oresult test
     will search for the string "oresult test" backward

     the string
     -b " oresult test"
     will search for the string " oresult test" backward (note the starting space)

     the string [5]
      -b oresult test
     will search for the string " -b oresult test"
     (if there are any options they should start at position 0)

     Inside "" escaped are: \\, \n, \"
     Use '' to eliminate \\, \n or \" expantion

     m/s prefixes turn on regular expressions flag
*/
BOOLEAN ParseFindInput(const char *psLine, TSearchContext *pstSearchContext)
{
  const char *pos;
  char *p;
  char sBuf[_MAX_PATH];
  BOOLEAN bFirst;
  BOOLEAN bQuoted;

  /* Set default values */
  ClearSearchContext(pstSearchContext);
  pstSearchContext->bCaseSensitive = FALSE;  /* ignore case */
  pstSearchContext->nDirection = 1;
  pstSearchContext->bRegularExpr = FALSE;
  pstSearchContext->bPrompt = TRUE;

  if (psLine[0] == '-' || psLine[0] == '\"')
    goto _parse_components;
  /* case [5] */
  if (ParseSearchPattern(psLine, pstSearchContext) != 0)
    return FALSE;
  return TRUE;

  ASSERT(strlen(psLine) < _MAX_PATH);
  strcpy(sBuf, psLine);

_parse_components:
  pos = ExtractComponent(psLine, sBuf, &bQuoted);
  p = sBuf;
  /*
  We need to check the first component extracted:
  if it is a valid option followed by space then this
  is a multi-component line
  otherwise the whole line is a text to be searched for.
  */
  bFirst = TRUE;
  while (1)
  {
    if (bQuoted)
    {
_a_search_pattern:
      /* This is a search pattern */
      if (ParseSearchPattern(p, pstSearchContext) != 0)
        return FALSE;
      goto _next_token;
    }
    if (*p != '-')
      goto _a_search_pattern;
    switch (*(p + 1))
    {
      case 'i':
        if (*(p + 3) != ' ' && *(p + 3) != '\0')  /* switch can be only 1 character */
        {
          /* invalid switch for -i option */
          PrepareInvalidSwitchMsg(pstSearchContext->sError, "-i", p + 2);
          return FALSE;
        }
        switch (*(p + 2))
        {
          case '\0':
          case '+':
            pstSearchContext->bCaseSensitive = FALSE;  /* ignore case */
            break;
          case '-':
            pstSearchContext->bCaseSensitive = TRUE;
            break;
          default:
            /* invalud switch for -i option */
            PrepareInvalidSwitchMsg(pstSearchContext->sError, "-i", p + 2);
            return FALSE;
        }
        break;

      case 'r':
        if (*(p + 3) != ' ' && *(p + 3) != '\0')  /* switch can be only 1 character */
        {
          /* invalid switch for -r option */
          PrepareInvalidSwitchMsg(pstSearchContext->sError, "-r", p + 2);
          return FALSE;
        }
        switch (*(p + 2))
        {
          case '\0':
          case '+':
            pstSearchContext->bRegularExpr = TRUE;
            break;
          case '-':
            pstSearchContext->bRegularExpr = FALSE;
            break;
          default:
            /* invalud switch for -r option */
            PrepareInvalidSwitchMsg(pstSearchContext->sError, "-r", p + 2);
            return FALSE;
        }
        break;

      case 'b':
        if (*(p + 3) != ' ' && *(p + 3) != '\0')  /* switch can be only 1 character */
        {
          /* invalid switch for -b option */
          PrepareInvalidSwitchMsg(pstSearchContext->sError, "-b", p + 2);
          return FALSE;
        }
        switch (*(p + 2))
        {
          case '\0':
          case '+':
            pstSearchContext->nDirection = -1;
            break;
          case '-':
            pstSearchContext->nDirection = 1;
            break;
          default:
            /* invalud switch for -b option */
            PrepareInvalidSwitchMsg(pstSearchContext->sError, "-b", p + 2);
            return FALSE;
        }
        break;

      case 'n':
        if (*(p + 3) != '\0')  /* switch can be only 1 character */
        {
          /* invalid switch for -n option */
          PrepareInvalidSwitchMsg(pstSearchContext->sError, "-n", p + 2);
          return FALSE;
        }
        switch (*(p + 2))
        {
          case '\0':
          case '+':
            pstSearchContext->bPrompt = FALSE;
            break;
          case '-':
            pstSearchContext->bPrompt = TRUE;
            break;
          default:
            /* invalud switch for -b option */
            PrepareInvalidSwitchMsg(pstSearchContext->sError, "-b", p + 2);
            return FALSE;
        }
        break;

      default:
        /* invalid option */
        sprintf(pstSearchContext->sError, sInvalidSearchModifier, p);
        return FALSE;
    }
_next_token:
    if (pos != NULL)
    {
      pos = ExtractComponent(pos, sBuf, &bQuoted);
      p = sBuf;
    }
    else
      return TRUE;
  }
}

/* ************************************************************************
   Function: AskReplace
   Description:
     0 - replace
     1 - do not replace
     2 - cancel operation
     3 - replace the rest of the occurences
*/
int AskReplace(dispc_t *disp)
{
  disp_event_t ev;
  unsigned long Key;

  ConsoleMessageProc(disp, NULL, MSG_WARNING | MSG_STATONLY, NULL, sAskReplace);
  while (1)
  {
    do
    {
      disp_event_read(disp, &ev);
    }
    while (ev.t.code != EVENT_KEY);
    Key = ev.e.kbd.key;
    switch (NO_ASC(Key))
    {
      case KEY(0, kbY):
      case KEY(0, kbEnter):
        return 0;
      case KEY(0, kbN):
        return 1;
      case KEY(0, kbEsc):
        return 2;
      case KEY(0, kbA):
        return 3;
    }
  }
}

#if 0 /* TODO: this must be reworked with the container interface */
/* ************************************************************************
   Function: ShowFile
   Description:
     Show file on the screen.
     This is called to show the file prior asking the user for the
     "Replace? (Y/N/ESC/Rest)"
*/
void ShowFile(dispc_t *disp)
{
  TFile *pCurFile;

  pCurFile = GetCurrentFile();

#if 0
  if (pCurFile->nCol != pCurFile->nPrevCol ||
    pCurFile->nRow != pCurFile->nPrevRow)
    OnCursorPosChanged(pCurFile);
  pCurFile->nPrevCol = pCurFile->nCol;
  pCurFile->nPrevRow = pCurFile->nRow;
#endif

  ConsoleUpdatePage(pCurFile, 0, 0, ScreenWidth, ScreenHeight - 1,
    SyntaxPaletteStart, NULL);
  ConsoleUpdateStatusLine(pCurFile);
  GotoXY(0 + pCurFile->x, 0 + pCurFile->y);
}
#endif

/* ************************************************************************
   Function: HelpFunc
   Description:
*/
static void HelpFunc(void *pCtx)
{
  ShowContextHelp(&stCtxHelp);
}

/* ************************************************************************
   Function: CmdEditFind
   Description:
*/
void CmdEditFind(void *pCtx)
{
  static char sFind[_MAX_PATH] = {'\0'};
  char sWordUnderCursor[_MAX_PATH];
  TFile *pFile;
  BOOLEAN bResult;
  disp_wnd_param_t wnd_param;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));
  pFile = CMDC_PFILE(pCtx);

  GetWordUnderCursor(CMDC_PFILE(pCtx), sWordUnderCursor, sizeof(sWordUnderCursor));
  if (sWordUnderCursor[0] != '\0')
    strcpy(sFind, sWordUnderCursor);

  HelpPushContext(&stCtxHelp, pCtxSep, pCtxSep, NULL);
  HelpPushContext(&stCtxHelp, "pline_edit", "Help on ~l~ine edit functions", NULL);
  HelpPushContext(&stCtxHelp, "search", "Help on ~s~earch command", NULL);
  HelpPushContext(&stCtxHelp, "reguexpr", "Help on ~r~egular expressions", NULL);

  bResult = EnterLn(sPromptFind, sFind, sizeof(sFind) - 1, pFindHistory, NULL, HelpFunc, NULL, FALSE, disp);
  HelpPopContext(&stCtxHelp, 4);
  if (!bResult)
    goto _exit;

  if (!ParseFindInput(sFind, &stSearchContext))
  {
    ConsoleMessageProc(disp, NULL, MSG_ERROR | MSG_OK, NULL, stSearchContext.sError);
    goto _exit;
  }
_search:
  pFile->bShowBlockCursor = TRUE;
  pFile->bUpdateLine = TRUE;
  disp_wnd_get_param(0, &wnd_param);  /* TODO: supply disp! */
  Find(pFile, stSearchContext.nDirection, wnd_param.width, &stSearchContext);
  if (!stSearchContext.bSuccess)
    goto _exit;
  if (stSearchContext.nNumRepl == 0)  /* Doint replace? */
    goto _exit;
  /* ShowFile(); TODO: restore ShowFile */
  if (stSearchContext.bPrompt)
  {
    switch (AskReplace(disp))
    {
      case 1:  /* Do not change this particular occurrence */
        goto _search;
        break;
      case 2:  /* Canceled operation */
        goto _exit;
      case 3:  /* Replace All */
        stSearchContext.bPrompt = FALSE;
	break;
    }
  }
  if (!Replace(&stSearchContext, pFile))
    goto _exit;  /* Operation failed (no-memory is one probability) */
  goto _search;  /* Search for the next occurence */

_exit:
  pFile->bShowBlockCursor = FALSE;
  pFile->bUpdateLine = TRUE;
  pFile->bUpdateStatus = TRUE;
}

/* ************************************************************************
   Function: CmdEditFindInFiles
   Description:
*/
void CmdEditFindInFiles(void *pCtx)
{
  static char sFindInFiles[_MAX_PATH] = {'\0'};
  char sWordUnderCursor[_MAX_PATH];
  TDocType *pDocType;
  int nWindow;
  disp_event_t ev;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  GetWordUnderCursor(CMDC_PFILE(pCtx), sWordUnderCursor, sizeof(sWordUnderCursor));
  if (sWordUnderCursor[0] != '\0')
  {
    strcpy(sFindInFiles, sWordUnderCursor);
    pDocType = DetectDocument(DocumentTypes, CMDC_PFILE(pCtx)->sFileName);
    if (pDocType != NULL)
    {
      strcat(sFindInFiles, " ");
      strcat(sFindInFiles, pDocType->sExt);
    }
  }

  if (!EnterLn(sPromptFindInFiles, sFindInFiles, sizeof(sFindInFiles) - 1, pFindInFilesHistory, NULL, NULL, NULL, FALSE, disp))
  {
    disp_event_clear(&ev);
    ev.t.code = EVENT_USR;
    ev.t.user_msg_code = MSG_INVALIDATE_SCR;
    ev.data1 = 0;  /* TODO: wrkspace here! */
    ContainerHandleEvent(&stRootContainer, &ev);
    return;
  }

  FindInFiles(disp, sFindInFiles, &nWindow);
  if (nWindow == 1)
    CmdWindowFindInFiles1(NULL);
  else
    if (nWindow == 2)
      CmdWindowFindInFiles2(NULL);
    else
      ASSERT(0);  /* invalid FindInFiles() output */
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

