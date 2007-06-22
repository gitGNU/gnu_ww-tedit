/*

File: fnavcmd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 24th December, 1998
Descrition:
  File navigation commands.

*/

#include "global.h"
#include "disp.h"
#include "palette.h"
#include "l2disp.h"
#include "heapg.h"  /* for occasional use of VALIDATE_HEAP() */
#include "memory.h"
#include "umenu.h"
#include "filenav.h"
#include "wrkspace.h"
#include "options.h"
#include "parslogs.h"
#include "bookmcmd.h"
#include "cmdc.h"
#include "fnavcmd.h"
#include "main2.h"

/* ************************************************************************
   Function: CmdWindowSwap
   Description:
     Acts like Windows CUI Ctrl+Tab, Ctrl+Shift+Tab keys.
     Pressing Ctrl+Tab and releasing Ctrl will swap among two files.
     Pressing Ctrl+Tab without releaseing Ctrl couple of times will
     navigate among all the files in the memory, pressing Shift in
     this combination will navigate back.
*/
void CmdWindowSwap(void *pCtx)
{
  if (pCurrentContainer->pView->bDockedView)
    MoveToFileView();

  /* TODO: disp, shift state should be stored at the workspace
  every time we read a keyboard event */
  //SwapFile(pFilesInMemoryList, (BOOLEAN)((ShiftState & kbShift) != 0));

  CMDC_PFILE(pCtx)->bUpdatePage = TRUE;
  CMDC_PFILE(pCtx)->bUpdateStatus = TRUE;
  /* TODO: disp, This flag should be exported to the workspace structure*/
  //bCtrlReleased = FALSE;
}

/* ************************************************************************
   Function: CmdWindowSwap
   Description:
*/
void CmdWindowUserScreen(void *pCtx)
{
#if 0
  ShowUserScreen();
  while (ReadKey() == 0xffff)
    ;  /* Ignore timer events */
  ((TFile *)pCtx)->bUpdatePage = TRUE;
#endif
}

/*
Below are variables that are passed to PutFileItem() call-back function
that prepares file names to be displayed.
*/
static char **ppIndex;
static int nCurIndex;
static int nCurItem;  /* Mark the current file index here */
static int nMaxWidth;

/* ************************************************************************
   Function: PutFileItem
   Description:
     Adds a file to pIndex
     TODO: CONTEXT
*/
static BOOLEAN PutFileItem(TFile *pFile, void *pContext)
{
  char sFileName[_MAX_PATH];
  char sCopy[10];
  char *p;
  int nLen;

  ShrinkPath(pFile->sFileName, sFileName, 0, TRUE);
  nLen = strlen(sFileName);
  p = alloc(nLen + 5);
  if (p == NULL)
    return FALSE;
  strcpy(p, sFileName);
  if (pFile->nCopy > 0)
  {
    sprintf(sCopy, "%d", pFile->nCopy + 1);
    /* was itoa(pFile->nCopy + 1, sCopy, 10); */
    strcat(p, ":");
    strcat(p, sCopy);
  }
  if (pFile->bChanged)
    strcat(p, " *");
  if (pFile == GetCurrentFile())
    nCurItem = nCurIndex;  /* Save the index number of the cur file */
  nLen = strlen(p);
  if (nLen > nMaxWidth)
    nMaxWidth = nLen;
  ppIndex[nCurIndex++] = p;
  return TRUE;
}

/* ************************************************************************
   Function: CmdWindowList
   Description:
*/
void CmdWindowList(void *pCtx)
{
  char **p;
  int i;
  char sResult[_MAX_PATH];
  BOOLEAN bResult;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  ppIndex = alloc(sizeof(char *) * (GetNumberOfFiles() + 1));
  if (ppIndex == NULL)
    return;  /* The no-memory error will be indicated in mainloop */

  nCurIndex = 0;
  nCurItem = -1;
  nMaxWidth = 0;
  FileListForEach(pFilesInMemoryList, PutFileItem, TRUE, NULL);
  if (nCurIndex == GetNumberOfFiles())
  {
    ASSERT(nCurIndex == GetNumberOfFiles());
    ppIndex[nCurIndex] = NULL;
    ASSERT(nCurItem >= 0);
    if (nMaxWidth < 15)
      nMaxWidth = 17;
    if (nMaxWidth > 45)
      nMaxWidth = 45;
    bResult = UMenu(disp_wnd_get_width(disp) - nMaxWidth - 14, 3, 1,
      disp_wnd_get_height(disp) - 8,
      nMaxWidth, "Files",  (const char **)ppIndex,
      sResult, &nCurItem, NULL, NULL, coUMenu, disp);
    if (bResult)
      SetTopFileByLoadNumber(pFilesInMemoryList, nCurItem);
  }
  p = ppIndex;
  for (i = 0; i < nCurIndex; ++i)
    s_free(*p++);
  s_free(ppIndex);
}

/* ************************************************************************
   Function: ActivateFile
   Description:
     Activates file as set in parameter nFile.
     This function is usually invoked in the following consequence:
     1. User selects a file from Window menu.
     2. Menu invokes CmdWindowX() depending on menu selection.
     3. CmdWindowX() calls ActivateFile(X) with proper parameter.
     As Window menu is composed depending on bConsequtiveWinFiles option
     ActivateFile() works being set by the same option.

     Q: What is the idea behind bConsequtiveWinFiles?
     A: In the new version of VisualC (6.0) Window menu shows all the
     files in their Z-order of selection. In the older versions of VisualC
     and BorlandC IDEs, Window menu showed files in the order as they
     were loaded. So bConsequtiveWinFiles is provided to smoothly
     switch among two version of composing Window menu and WindowList
     menu.
*/
static void ActivateFile(TFileList *pFileList, int nFile)
{
  if (nFile >= pFileList->nNumberOfFiles)
    return;

  if (bConsequtiveWinFiles)
    SetTopFileByLoadNumber(pFileList, nFile);
  else
    SetTopFileByZOrder(pFileList, nFile);
}

/* ************************************************************************
   Function: CmdWindow1
   Description:
     This functions is usually assigned to Window|File1 submenu.
     In BorlandIDE is assigned to a key combination Alt+1 too.
*/
void CmdWindow1(void *pCtx)
{
  ActivateFile(pFilesInMemoryList, 0);
}

/* ************************************************************************
   Function: CmdWindow2
   Description:
     This functions is usually assigned to Window|File2 submenu.
     In BorlandIDE is assigned to a key combination Alt+2 too.
*/
void CmdWindow2(void *pCtx)
{
  ActivateFile(pFilesInMemoryList, 1);
}

/* ************************************************************************
   Function: CmdWindow3
   Description:
     This functions is usually assigned to Window|File3 submenu.
     In BorlandIDE is assigned to a key combination Alt+3 too.
*/
void CmdWindow3(void *pCtx)
{
  ActivateFile(pFilesInMemoryList, 2);
}

/* ************************************************************************
   Function: CmdWindow4
   Description:
     This functions is usually assigned to Window|File4 submenu.
     In BorlandIDE is assigned to a key combination Alt+4 too.
*/
void CmdWindow4(void *pCtx)
{
  ActivateFile(pFilesInMemoryList, 3);
}

/* ************************************************************************
   Function: CmdWindow5
   Description:
     This functions is usually assigned to Window|File5 submenu.
     In BorlandIDE is assigned to a key combination Alt+5 too.
*/
void CmdWindow5(void *pCtx)
{
  ActivateFile(pFilesInMemoryList, 4);
}

/* ************************************************************************
   Function: CmdWindow6
   Description:
     This functions is usually assigned to Window|File6 submenu.
     In BorlandIDE is assigned to a key combination Alt+6 too.
*/
void CmdWindow6(void *pCtx)
{
  ActivateFile(pFilesInMemoryList, 5);
}

/* ************************************************************************
   Function: CmdWindow7
   Description:
     This functions is usually assigned to Window|File7 submenu.
     In BorlandIDE is assigned to a key combination Alt+7 too.
*/
void CmdWindow7(void *pCtx)
{
  ActivateFile(pFilesInMemoryList, 6);
}

/* ************************************************************************
   Function: CmdWindow8
   Description:
     This functions is usually assigned to Window|File8 submenu.
     In BorlandIDE is assigned to a key combination Alt+8 too.
*/
void CmdWindow8(void *pCtx)
{
  ActivateFile(pFilesInMemoryList, 7);
}

/* ************************************************************************
   Function: CmdWindow9
   Description:
     This functions is usually assigned to Window|File9 submenu.
     In BorlandIDE is assigned to a key combination Alt+9 too.
*/
void CmdWindow9(void *pCtx)
{
  ActivateFile(pFilesInMemoryList, 8);
}

/* ************************************************************************
   Function: CmdWindow10
   Description:
     This functions is usually assigned to Window|File10 submenu.
     In BorlandIDE is assigned to a key combination Alt+10 too.
*/
void CmdWindow10(void *pCtx)
{
  ActivateFile(pFilesInMemoryList, 9);
}


/* ************************************************************************
   Function: CmdToolParseLogFile
   Description:
     Will parse a customer supplied log file into bookmarks. Makes it
     available for navigation of error messages.
*/
void CmdToolParseLogFile(void *pCtx)
{
  BMListDisposeBMSet(&stOutput);
  ParseLogFile("errlog.txt", &stOutput);
  CmdWindowOutput(NULL);
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

