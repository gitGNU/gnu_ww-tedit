/*

File: filemenu.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 22nd December, 1998
Descrition:
  Implements a file input name prompt along with a file menu
  to navigate.

*/

#include "global.h"
#include "memory.h"
#include "disp.h"
#include "palette.h"
#include "path.h"
#include "l1def.h"
#include "l2disp.h"
#include "defs.h"
#include "umenu.h"
#include "enterln.h"
#include "defs.h"
#include "filemenu.h"
#include "wrkspace.h"

#include "findf.h"

#ifdef WIN32
#include <io.h>
#endif

#ifdef MSDOS
#include <dos.h>
#endif

#include <errno.h>

/* from searchf.c */
extern BOOLEAN MatchFile(const char *psFile, const char *psMasks);

/* ************************************************************************
   Function: ReadDir
   Description:
     Reads a directory specified by sPath.
   On exit:
     TRUE - read ok and	pppIndex is an index array to file names.
     FALSE - I/O error reading the dir examine errno.
*/
static BOOLEAN ReadDir(const char *sPath, TListEntry *flist, int *nFiles)
{
  char sPathOnly[_MAX_PATH];
  char sMaskOnly[_MAX_PATH];
  char sReadAllMask[_MAX_PATH];
  char sBuf[_MAX_PATH];
  TListEntry *pHeapFile;
  struct FF_DAT *ff_dat;
  struct findfilestruct ff;
  char *p;

  ASSERT(sPath != NULL);
  ASSERT(flist != NULL);
  ASSERT(nFiles	!= NULL);

  INITIALIZE_LIST_HEAD(flist);
  *nFiles = 0;

  /* Separate path and filename */
  if (!FSplit(sPath, sPathOnly, sMaskOnly, sDirMask, FALSE, FALSE))
    return (FALSE);

  /*
  Read the directories first
  */
  strcpy(sReadAllMask, sPathOnly);
  strcat(sReadAllMask, sAllMask);  /* "*.*" */

  ff_dat = find_open(sReadAllMask, FFIND_DIRS);
  if (ff_dat == NULL)
    return (FALSE);

  while (find_file(ff_dat, &ff) == 0)
  {
    if (strcmp(ff.filename, ".") == 0)
      continue;  /* Skip "." directory */

    /* Compose a dir entry */
    strcpy(sBuf, ff.filename);
    p = strchr(sBuf, '\0');
    ASSERT(p != NULL);
    *p++ = PATH_SLASH_CHAR;
    *p = '\0';
    pHeapFile = s_alloc(strlen(sBuf) + 1 + sizeof(TListEntry));
    INSERT_TAIL_LIST(flist, pHeapFile);
    strcpy((char *)(pHeapFile + 1), sBuf);
    ++*nFiles;
  }

  find_close(ff_dat);

  /*
  Read the file items of the directory
  */
  ff_dat = find_open(sReadAllMask, FFIND_FILES);
  if (ff_dat == NULL)
    return (FALSE);

  while (find_file(ff_dat, &ff) == 0)
  {
    /* match against the mask */
    if (!MatchFile(ff.filename, sMaskOnly))
      continue;
    /* Compose a file entry */
    pHeapFile = s_alloc(strlen(ff.filename) + 1 + sizeof(TListEntry));
    INSERT_TAIL_LIST(flist, pHeapFile);
    strcpy((char *)(pHeapFile + 1), ff.filename);
    ++*nFiles;
  }

  find_close(ff_dat);

  return TRUE;
}

/* ************************************************************************
   Function: cmpfiles
   Description:
     Call-back function invoked by qsort.
     Compares two filenames. Directories weight more.
*/
static int cmpfiles(const void *_p1, const void *_p2)
{
  char ** p1 = (char **)_p1;
  char ** p2 = (char **)_p2;
  char *slashp1;
  char *slashp2;

  slashp1 = strchr(*p1, '\0') - 1;
  slashp2 = strchr(*p2, '\0') - 1;
  /* Put dir entries at first place */
  if (*slashp1 == PATH_SLASH_CHAR)
  {
    if (*slashp2 != PATH_SLASH_CHAR)
      return -1;
  }
  else
    if (*slashp2 == PATH_SLASH_CHAR)
      if (*slashp1 != PATH_SLASH_CHAR)
	return 1;

  return stricmp(*p1, *p2);
}

/* ************************************************************************
   Function: MakeFileIndex
   Description:
     Produces an array of pointers deriving it from an list of strings.
   Returns:
     Pionter to an array allocated in the heap.
     End of array marked with an item of NULL.
*/
static char **MakeFileIndex(TListEntry *flist, int nFiles)
{
  char **ppIndex;
  char **p;
  TListEntry *pHeapFile;
  #ifdef _DEBUG
  int _nFiles;
  #endif

  ASSERT(flist != NULL);

  if (nFiles == 0)
    return (NULL);  /* No file entries */

  /* Make the list of file items as index array */
  ppIndex = s_alloc(sizeof(char *) * (nFiles + 1));  /* +1 for one NULL marker */
  p = ppIndex;
  pHeapFile = flist->Flink;
  #ifdef _DEBUG
  _nFiles = nFiles;
  #endif
  while (!END_OF_LIST(flist, pHeapFile))
  {
    *p = (char *)(pHeapFile + 1);
    p++;
    pHeapFile = pHeapFile->Flink;
    #ifdef _DEBUG
    --_nFiles;
    ASSERT(_nFiles >= 0);
    #endif
  }

  *p = NULL;  /* End of index list */

  qsort(ppIndex, nFiles, sizeof(char *), cmpfiles);

  return ppIndex;
}

/* ************************************************************************
   Function: _strrchr
   Description:
     Safe search back.
*/
static char *_strrchr(char *s, char *p, char c)
{
  while (p >= s)
  {
    if (*p == c)
      return p;
    --p;
  }
  return NULL;
}

/* ************************************************************************
   Function: FileMenu
   Description:
     Shows a menu with files.
   On exit:
     FALSE - exit with ESC or I/O error.
     TRUE - sPath holds a file name.
*/
BOOLEAN FileMenu(char *sPath, dispc_t *disp)
{
  TListEntry flist;
  TListEntry *pFileItem;
  char **ppIndex;
  char **ppItem;
  int nFiles;
  void *pSaveStatLn;
  BOOLEAN bExitCode;
  char sResult[_MAX_PATH];
  char sSplitPath[_MAX_PATH];
  char sMask[_MAX_PATH];
  char sTitle[_MAX_PATH];
  int nStartIndex;
  char *p;
  char sUpDir[_MAX_PATH];
  disp_wnd_param_t wnd_param;

  ASSERT(sPath != NULL);
  ASSERT(strlen(sPath) < _MAX_PATH);

  bExitCode = TRUE;
  sUpDir[0] = '\0';

Read:
  pSaveStatLn = SaveStatusLine(disp);
  DisplayStatusStr(sReadingDir, coStatus, TRUE, TRUE, disp);

  /* Make a list of files in the directory */
  if (!ReadDir(sPath, &flist, &nFiles))
  {
    ConsoleMessageProc(disp, NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, NULL, NULL);
    RestoreStatusLine(pSaveStatLn, disp);
    bExitCode = FALSE;
    goto Exit;
  }
  RestoreStatusLine(pSaveStatLn, disp);

  if (nFiles == 0)
  {
    bExitCode = FALSE;
    goto Exit;
  }

  /* Make an array of pointers deriving from the list */
  ppIndex = MakeFileIndex(&flist, nFiles);

  nStartIndex = 0;  /* Start with current item 0 */
  if (sUpDir[0] != '\0')
  {
    /* Go to the position of a selected item -- sUpDir */
    for (ppItem = ppIndex; *ppItem != NULL; ++ppItem)
    {
      if (filestrcmp(*ppItem, sUpDir) == 0)
      {
        nStartIndex = ppItem - ppIndex;
        break;
      }
    }
  }
  ShrinkPath(sPath, sTitle, 40, FALSE);  /* Prepare sPath to fit in the menu title */
  DisplayStatusStr2(sStatMenu, coStatusTxt, coStatusShortCut, disp);

  disp_wnd_get_param(disp, &wnd_param);
  bExitCode = UMenu(25, 3, 2, wnd_param.height - 7, 24,
                    sTitle, (const char **)ppIndex, sResult,
                    &nStartIndex, NULL, NULL, coUMenu, disp);

  /* Free the list of files */
  while (!IS_LIST_EMPTY(&flist))
  {
    pFileItem = REMOVE_HEAD_LIST(&flist);
    s_free(pFileItem);
  }

  s_free(ppIndex);

  if (bExitCode)
  {
    /* Separate path and filename */
    if (!FSplit(sPath, sSplitPath, sMask, sDirMask, FALSE, FALSE))
    {
      return (FALSE);
    }
    p = strchr(sResult, '\0') - 1;
    if (*p == PATH_SLASH_CHAR)
    {
      /* A sub dir */
      sUpDir[0] = '\0';
      if (strncmp(sResult, "..", 2) == 0)  /* We are going a level up */
      {
        /* Isolate the last directory into sUpDir */
        p = strchr(sSplitPath, '\0');
        --p;
        p = _strrchr(sSplitPath, p, PATH_SLASH_CHAR);
        if (p == NULL)
          goto _combine_path;
        /* we are now ar \UpDir\..\ */
        --p;
        p = _strrchr(sSplitPath, p, PATH_SLASH_CHAR);
        if (p == NULL)
          goto _combine_path;
        strcpy(sUpDir, p + 1);
      }
_combine_path:
      strcat(sSplitPath, sResult);
      strcat(sSplitPath, sMask);
      ConsoleMessageProc(disp, NULL, MSG_STATONLY | MSG_INFO, NULL, sExtractingFullName);
      GetFullPath(sSplitPath);
      strcpy(sPath, sSplitPath);
      goto Read;
    }
    else
    {
      strcat(sSplitPath, sResult);
      strcpy(sPath, sSplitPath);
    }
  }

Exit:
  return bExitCode;
}

/* ************************************************************************
   Function: GetDocumentMask
   Description:
     Shows a menu and
*/
char *GetDocumentMask(void)
{
  const char **pMenuItem;
  TDocType *pDocumentItem;
  const char *pMenu[MAX_DOCS];
  static char sResult[MAX_EXT_LEN];
  BOOLEAN bMenuResult;
  int nCurItem;
  disp_wnd_param_t wnd_param;

  /*
  Prepares an index array containing all the document type set
  items to be used by UMenu().
  */
  pMenuItem = pMenu;
  for (pDocumentItem = DocumentTypes; !IS_END_OF_DOC_LIST(pDocumentItem); ++pDocumentItem)
    *pMenuItem++ = pDocumentItem->sExt;

  *pMenuItem = NULL;  /* Point the end menu index */

  /*
  Display the menu for the user to select
  */
  nCurItem = 0;

  /* TODO: pass disp here */
  disp_wnd_get_param(0, &wnd_param);
  bMenuResult = UMenu(5, wnd_param.height - 13, 1, 8, 20, " select a mask ", pMenu,
                      sResult, &nCurItem, NULL, NULL, coUMenu, 0);
  if(bMenuResult)
    return sResult;
  return NULL;
}

/* ************************************************************************
   Function: InputFileName
   Description:
     Inputs a file name. psPorompt is the input	line prompting string.
     psResult is two way interface. When InputFileName is invoked this
     is the default string for editing, on exit here is stored the result.
     pHist is the history to store the inputs or to prompt for fast input.
   Returns:
     TRUE - a name is entered.
     FALSE - exit with ESC.
*/
BOOLEAN InputFileName(const char *sPrompt, char *sResult, THistory *pHist,
  const char *sDefaultMask, dispc_t *disp)
{
  char sPath[_MAX_PATH];
  char sDPath[_MAX_PATH];
  char sDFile[_MAX_PATH];
  char sSearchMask[_MAX_PATH];
  char sTemp[_MAX_PATH];
  int nBlankSegmentLen;
  disp_event_t ev;

  ASSERT(sPrompt != NULL);
  ASSERT(sResult != NULL);
  ASSERT(strlen(sPrompt) < 25);
  ASSERT(sDefaultMask != NULL);

  strcpy(sPath, sResult);

  ReEnter:
  if (!EnterLn(sPrompt, sPath, _MAX_PATH, pHist, GetDocumentMask,
	  NULL, NULL, TRUE, disp))
  {
    disp_event_clear(&ev);
    ev.t.code = EVENT_USR;
    ev.t.user_msg_code = MSG_INVALIDATE_SCR;
    ev.data1 = stRootContainer.wrkspace;
    ContainerHandleEvent(&stRootContainer, &ev);
    return FALSE;
  }

  /* Remove leading blanks from filename */
  nBlankSegmentLen = strspn(sPath, " ");
  if (nBlankSegmentLen > 0)
    strcpy(sPath, &sPath[nBlankSegmentLen]);

  ConsoleMessageProc(disp, NULL, MSG_STATONLY | MSG_INFO, NULL, sExtractingFullName);

  /*
  Analize the name
  1. If it is a directory name enter the directory with sDefaultMask by
  invoking FileMenu.
  2. If has wildcards invoke to process the FileMenu as well.
  */
  if (!FSplit(sPath, sDPath, sDFile, sDefaultMask, TRUE, FALSE))
  {
DisplayError:
    ConsoleMessageProc(disp, NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, sPath, NULL);
    goto ReEnter;
  }

  if (HasWild(sDFile))  /* HasWild */
  {
    strcpy(sResult, sDPath);
    strcat(sResult, sDFile);
    strcpy(sSearchMask, sDFile);
    if (!FileMenu(sResult, disp))
      goto ReEnter;
    /* Put in history the file name and the path+mask */
    if (!FSplit(sResult, sDPath, sDFile, sDefaultMask, FALSE, FALSE))
      goto DisplayError;
    strcat(sDPath, sSearchMask);
    ShrinkPath(sDPath, sTemp, 0, FALSE);  /* Exclude the full path */
    if (strcmp(sTemp, sSearchMask) != 0)  /* Only if not in the cur dir */
      if (pHist != NULL)
        AddHistoryLine(pHist, sTemp);
    ShrinkPath(sResult, sTemp, 0, FALSE);  /* Exclude the full path */
    if (pHist != NULL)
      AddHistoryLine(pHist, sTemp);
    strcpy(sResult, sTemp);  /* Exclude the full path from psResult as well */
  }
  else
  {
    /* we are operating on the real file name (latters case preserved) */
    strcpy(sPath, sDPath);
    strcat(sPath, sDFile);
    ShrinkPath(sPath, sResult, 0, FALSE);  /* Exclude the full path */
  }

  return TRUE;
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

