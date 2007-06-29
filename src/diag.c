/*

File: diag.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 13th December, 1998
Descrition:
  Various diag functions, routines and commands.

*/

#include "global.h"
#include "scr.h"
#include "kbd.h"
#include "keydefs.h"
#include "l1opt.h"
#include "main2.h"
#include "heapg.h"
#include "memory.h"
#include "filenav.h"
#include "mru.h"
#include "ini.h"
#include "undo.h"
#include "wrkspace.h"
#include "filecmd.h"
#ifdef WIN32
#include "winclip.h"
#endif
#include "search.h"
#include "diag.h"

/* ************************************************************************
   Function: DiagContinue
   Description:
     Displays "Continue..."
*/
BOOLEAN DiagContinue(int nCount)
{
  DWORD Key;

  if (nCount % (ScreenHeight - 3))
    return TRUE;

  PrintString("Continue...(<ESC>-cancel)\n");

  while ((Key = ReadKey()) == 0xffff)
    ;  /* Ignore recovery time indicator */

  return Key != KEY(0, kbEsc);
}

/* ************************************************************************
   Function: DiagKeyNames
   Description:
*/
void DiagKeyNames(void)
{
  DWORD Key;
  char sKeyName[35];
  #ifdef UNIX
  extern BOOLEAN bTraceKbd;
  #endif
  struct event stEvent;
  BOOLEAN bQuit;
  int nButtonState;
  int nB1;
  int nB2;
  int nB3;

  PrintString("Press <ESC> to cancel\n");
  #ifdef UNIX
  bTraceKbd = TRUE;
  #endif
  bQuit = FALSE;
  while (!bQuit)
  {
    ReadEvent(&stEvent);
    switch (stEvent.t.code)
    {
      case EVENT_KEY:
        Key = stEvent.e.nKey;
        GetKeyName(Key, sKeyName);
        PrintString("%s %#lx state:%#x\n", sKeyName, NO_SH_STATE(Key), SH_STATE(Key));
        if (Key == KEY(0, kbEsc))
          bQuit = TRUE;
        break;
      case EVENT_RESIZE:
        PrintString("event WINDOW_BUFFER_SIZE_EVENT:\n"
          "new_width: %d, new_height: %d\n",
          stEvent.e.stNewSizeEvent.NewWidth,
          stEvent.e.stNewSizeEvent.NewHeight);
        break;
      case EVENT_MOUSE:
        nButtonState = stEvent.e.stMouseEvent.nButtonState;
        nB1 = (nButtonState & MOUSE_BUTTON1) != 0;
        nB2 = (nButtonState & MOUSE_BUTTON2) != 0;
        nB3 = (nButtonState & MOUSE_BUTTON3) != 0;
        PrintString("mouse: x=%d, y=%d, b=%x b1=%d, b2=%d, b3=%d, flags=%x\n",
          stEvent.e.stMouseEvent.x, stEvent.e.stMouseEvent.y,
          nButtonState, nB1, nB2, nB3,
          stEvent.e.stMouseEvent.nButtonData);
        if (stEvent.e.stMouseEvent.nFlags == MOUSE_DOUBLE_CLICK)
          PrintString("double-click\n");
        break;
      default:
        PrintString("event: %x\n", stEvent.t.code);
        break;
    }
  }
  #ifdef UNIX
  bTraceKbd = FALSE;
  #endif
}

/* ************************************************************************
   Function: DiagParameters
   Description:
*/
static void DiagParameters(void)
{
  PrintString("Parameters:\nModulePath: %s\nINIFile: %s\nMasterINIFile: %s\n",
    sModulePath, sINIFileName, sMasterINIFileName);
}

/* ************************************************************************
   Function: DiagHeap
   Description:
*/
static void DiagHeap(void)
{
#ifdef _DEBUG
  DWORD Key;
#endif

  #ifdef _DEBUG
_heap_menu:
  PrintString( "Heap statistics\n"
          "------------------\n"
  	      "~Used size:~   %ld\n"
          "~Peak size:~   %ld\n"
          "~Used blocks:~ %ld\n"
          "~Peak count:~  %ld\n"
          "~xmalloc():~   %ld\n"
          "~xfree():~     %ld\n"
          "~xrealloc():~  %ld\n\n",
         _dbg_UsedHeapSize,
         _dbg_MaxUsedHeapSize,
         _dbg_UsedBlocksCount,
         _dbg_MaxUsedBlocksCount,
         _dbg_xmallocCount,
         _dbg_xfreeCount,
         _dbg_xreallocCount );
  #ifdef HEAP_DBG
  PrintString("~bReturnNULL~ = %s (set to TRUE in order to simulate out of memory)\n", bReturnNULL ? "TRUE" : "FALSE");
  PrintString("~bTraceMemory~ = %s (set to TRUE in order to trace in DEBUG.LOG)\n", bTraceMemory ? "TRUE" : "FALSE");
  #endif
  #endif

  if (pSafetyPool)
    PrintString("SafetyPool not activated. %u bytes preserved.\n\n", SAFETY_POOL);
  else
    PrintString("SAFETY POOL ACTIVATED!\n\n");

  #ifdef HEAP_DBG
  PrintString("Heap:\n"
         "~[1]~Toggle_bReturnNULL\n"
         "~[2]~Toggle_bTraceMemory\n");

_wait_key:
  Key = ReadKey();

  if (Key == KEY(0, kbEsc))
  {
    PrintString("\n");
    return;
  }

  if (ASC(Key) == '1')
    bReturnNULL = !bReturnNULL;
  else
    if (ASC(Key) == '2')
    {
      TRACE1("%d\n", _dbg_UsedHeapSize);
      bTraceMemory = !bTraceMemory;
    }
    else
      goto _wait_key;

  goto _heap_menu;
  #endif
}

/* ************************************************************************
   Function: DiagFile
   Description:
*/
void DiagFile(void)
{
  DWORD Key;

_file_menu:
  PrintString("File: ~[D]~ump U~[n]~doIndex ~[F]~ilesList ~[M]~RUList ~[C]~liboard\n");
  PrintString("F~[u]~nctionList\n");
_wait_key:
  Key = ReadKey();

  if (Key == KEY(0, kbEsc))
  {
    PrintString("\n");
    return;
  }

  switch (ASC(Key))
  {
    case 'd':
    case 'D':
      DumpFile(GetCurrentFile());
      break;

    case 'n':
    case 'N':
      DumpUndoIndex(GetCurrentFile());
      break;

    case 'f':
    case 'F':
      DumpFileList(pFilesInMemoryList);
      break;

    case 'm':
    case 'M':
      DumpMRUList(pMRUFilesList);
      break;

    case 'c':
    case 'C':
      PrintString("Clipboard: ");
      #ifdef WIN32
      GetFromWindowsClipboard(&pClipboard);
      PrintString("extracted from Microsoft Windows clipboard\n");
      #endif
      DumpBlock(pClipboard, 2);
      break;

    case 'u':
    case 'U':
      DumpFuncList(GetCurrentFile());
      break;

    default:
      goto _wait_key;
  }

  PrintString("\n");
  goto _file_menu;
}

/* ************************************************************************
   Function: RecoveryRecord
   Description:
*/
void RecoveryRecord(void)
{
  TFile *pFile;

  pFile = GetCurrentFile();
  if (pFile->nCopy > 0)
  {
    PrintString("recovery record function is not permitted for copy >0");
    return;
  }
  if (!StoreRecoveryRecord(pFile))
    perror("StoreRecoveryRecord() failed");
  PrintString("stored\n");
}

/* ************************************************************************
   Function: CrashTest
   Description:
*/
void CrashTest(void)
{
  char *p;

  p = 0;
  *p = 1;
}

#if 0
#ifdef UNIX
#ifndef _NON_TEXT
#include <ncurses/term.h>

#define NCURSTEST

/* ************************************************************************
   Function: NCursesTest
   Description:
*/
void NCursesTest(void)
{
  char *s;

  s = tigetstr("kf1");
  PrintString("%p\n", s);
  if (s != -1 && s != 0)
    PrintString("%x:%x:%x:%x\n", s[0], s[1], s[2], s[3]);
  s = tigetstr("kf2");
  PrintString("%p\n", s);
  if (s != -1 && s != 0)
    PrintString("%x:%x:%x:%x\n", s[0], s[1], s[2], s[3]);
  s = tigetstr("kf3");
  PrintString("%p\n", s);
  if (s != -1 && s != 0)
    PrintString("%x:%x:%x:%x\n", s[0], s[1], s[2], s[3]);
}
#endif
#endif
#endif

#ifndef NCURSTEST
void NCursesTest(void)
{
}
#endif

/* ************************************************************************
   Function: CmdDiag
   Description:
*/
void CmdDiag(void *pCtx)
{
  DWORD Key;

  OpenSmallTerminal();

  PrintString("~version:~ %d.%d.%d\n", vermaj, vermin, ver_revision);
  PrintString("~build:~ [%d]\n", verbld);
  PrintString("~configuration file version:~ %d\n", vercfg);
  PrintString("~build date/time:~ %s; %s\n", datebld, timebld);
  PrintString("~definitions:~\n");
#ifdef _DEBUG
  PrintString("_DEBUG_\n");
#endif
#ifdef HEAP_DBG
  PrintString("HEAP_DBG=%d\n", HEAP_DBG);
#endif
#ifdef WIN32
  PrintString("WIN32\n");
#endif
#ifdef _MSC_VER
  PrintString("_MSC_VER=%d\n", _MSC_VER);
#endif
#ifdef MSDOS
  PrintString("MSDOS\n");
#endif
#ifdef DJGPP
  PrintString("DJGPP=%d, DJGPP_MINOR=%d\n", DJGPP, DJGPP_MINOR);
#endif
#ifdef LINUX
  PrintString("LINUX\n");
#endif
#ifdef UNIX
  PrintString("UNIX\n");
#endif

  PrintString("\ndiagnostic terminal:\n");

_main_menu:
  PrintString("~[F]~ile ~[H]~eap ~[P]~aram ~[K]~eys ~[R]~ecoveryStore ~[B]~ookmarks\n~[S]~earch"
    " ~[C]~ontainer Cr~[a]~sh ~[N]~curses\n");

_wait_key:
  Key = ReadKey();

  if (Key == KEY(0, kbEsc))
  {
    CloseSmallTerminal();
    ContainerHandleEvent(MSG_INVALIDATE_SCR, &stRootContainer, NULL);
    return;
  }

  switch (ASC(Key))
  {
    case 'f':
    case 'F':
      DiagFile();
      break;
    case 'k':
    case 'K':
      DiagKeyNames();
      break;
    case 'p':
    case 'P':
      DiagParameters();
      break;
    case 'h':
    case 'H':
      DiagHeap();
      break;
    case 'r':
    case 'R':
      RecoveryRecord();
      break;
    case 'B':
    case 'b':
      DumpBookmarks();
      break;
    case 's':
    case 'S':
      DumpSearchReplacePattern(&stSearchContext);
      break;
    case 'c':
    case 'C':
      DumpContainersTree(&stRootContainer, 0);
      break;
    case 'a':
    case 'A':
      CrashTest();
      break;
    case 'n':
    case 'N':
      NCursesTest();
      break;
    default:
      goto _wait_key;
  }

  goto _main_menu;
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

