/*

File: diag.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 13th December, 1998
Descrition:
  Various diag functions, routines and commands.

*/

#include "global.h"
#include "disp.h"
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
#include "cmdc.h"
#include "diag.h"

/* ************************************************************************
   Function: DiagContinue
   Description:
     Displays "Continue..."
*/
BOOLEAN DiagContinue(int nCount, dispc_t *disp)
{
  DWORD key;
  disp_event_t ev;

  if (nCount % (disp_wnd_get_height(disp) - 3))
    return TRUE;

  PrintString(disp, "Continue...(<ESC>-cancel)\n");

  do
  {
    disp_event_read(disp, &ev);
    key = 0;
    if (ev.t.code == EVENT_KEY)
      key = ev.e.kbd.key;
  }
  while (key== 0xffff);
  /* Ignore recovery time indicator */

  return key != KEY(0, kbEsc);
}

/* ************************************************************************
   Function: DiagKeyNames
   Description:
*/
static void DiagKeyNames(dispc_t *disp)
{
  DWORD Key;
  char sKeyName[35];
  #ifdef UNIX
  extern BOOLEAN bTraceKbd;
  #endif
  disp_event_t stEvent;
  BOOLEAN bQuit;
  int nButtonState;
  int nB1;
  int nB2;
  int nB3;

  PrintString(disp, "Press <ESC> to cancel\n");
  #ifdef UNIX
  bTraceKbd = TRUE;
  #endif
  bQuit = FALSE;
  while (!bQuit)
  {
    disp_event_read(disp, &stEvent);
    switch (stEvent.t.code)
    {
      case EVENT_KEY:
        Key = stEvent.e.kbd.key;
        disp_get_key_name(0, Key, sKeyName, sizeof(sKeyName));
        PrintString(disp, "%s %#lx state:%#x\n", sKeyName, NO_SH_STATE(Key), SH_STATE(Key));
        if (Key == KEY(0, kbEsc))
          bQuit = TRUE;
        break;
      case EVENT_RESIZE:
        PrintString(disp, "event WINDOW_BUFFER_SIZE_EVENT:\n"
          "new_width: %d, new_height: %d\n",
          stEvent.e.new_size.width,
          stEvent.e.new_size.height);
        break;
      case EVENT_MOUSE:
        nButtonState = stEvent.e.mouse.button_state;
        nB1 = (nButtonState & MOUSE_BUTTON1) != 0;
        nB2 = (nButtonState & MOUSE_BUTTON2) != 0;
        nB3 = (nButtonState & MOUSE_BUTTON3) != 0;
        PrintString(disp, "mouse: x=%d, y=%d, b=%x b1=%d, b2=%d, b3=%d, flags=%x\n",
          stEvent.e.mouse.x, stEvent.e.mouse.y,
          nButtonState, nB1, nB2, nB3,
          stEvent.e.mouse.button_data);
        if (stEvent.e.mouse.flags == MOUSE_DOUBLE_CLICK)
          PrintString(disp, "double-click\n");
        break;
      default:
        PrintString(disp, "event: %x\n", stEvent.t.code);
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
static void DiagParameters(dispc_t *disp)
{
  PrintString(disp, "Parameters:\nModulePath: %s\nINIFile: %s\nMasterINIFile: %s\n",
    sModulePath, sINIFileName, sMasterINIFileName);
}

/* ************************************************************************
   Function: DiagHeap
   Description:
*/
static void DiagHeap(dispc_t *disp)
{
#ifdef _DEBUG
  DWORD Key;
  disp_event_t ev;
#endif

  #ifdef _DEBUG
_heap_menu:
  PrintString(disp, "Heap statistics\n"
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
  PrintString(disp, "~bReturnNULL~ = %s (set to TRUE in order to simulate out of memory)\n", bReturnNULL ? "TRUE" : "FALSE");
  PrintString(disp, "~bTraceMemory~ = %s (set to TRUE in order to trace in DEBUG.LOG)\n", bTraceMemory ? "TRUE" : "FALSE");
  #endif
  #endif

  if (pSafetyPool)
    PrintString(disp, "SafetyPool not activated. %u bytes preserved.\n\n", SAFETY_POOL);
  else
    PrintString(disp, "SAFETY POOL ACTIVATED!\n\n");

  #ifdef HEAP_DBG
  PrintString(disp, "Heap:\n"
         "~[1]~Toggle_bReturnNULL\n"
         "~[2]~Toggle_bTraceMemory\n");

_wait_key:
  do
  {
    disp_event_read(disp, &ev);
  }
  while (ev.t.code != EVENT_KEY);
  Key = ev.e.kbd.key;

  if (Key == KEY(0, kbEsc))
  {
    PrintString(disp, "\n");
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
static void DiagFile(dispc_t *disp)
{
  DWORD Key;
  disp_event_t ev;

_file_menu:
  PrintString(disp, "File: ~[D]~ump U~[n]~doIndex ~[F]~ilesList ~[M]~RUList ~[C]~liboard\n");
  PrintString(disp, "F~[u]~nctionList\n");
_wait_key:
  do
  {
    disp_event_read(disp, &ev);
  }
  while (ev.t.code != EVENT_KEY);
  Key = ev.e.kbd.key;

  if (Key == KEY(0, kbEsc))
  {
    PrintString(disp, "\n");
    return;
  }

  switch (ASC(Key))
  {
    case 'd':
    case 'D':
      DumpFile(GetCurrentFile(), disp);
      break;

    case 'n':
    case 'N':
      DumpUndoIndex(GetCurrentFile(), disp);
      break;

    case 'f':
    case 'F':
      DumpFileList(pFilesInMemoryList, disp);
      break;

    case 'm':
    case 'M':
      DumpMRUList(pMRUFilesList, disp);
      break;

    case 'c':
    case 'C':
      PrintString(disp, "Clipboard: ");
      #ifdef WIN32
      GetFromWindowsClipboard(&pClipboard);
      PrintString(disp, "extracted from Microsoft Windows clipboard\n");
      #endif
      DumpBlock(pClipboard, 2, disp);
      break;

    case 'u':
    case 'U':
      DumpFuncList(GetCurrentFile(), disp);
      break;

    default:
      goto _wait_key;
  }

  PrintString(disp, "\n");
  goto _file_menu;
}

/* ************************************************************************
   Function: RecoveryRecord
   Description:
*/
static void RecoveryRecord(dispc_t *disp)
{
  TFile *pFile;

  pFile = GetCurrentFile();
  if (pFile->nCopy > 0)
  {
    PrintString(disp, "recovery record function is not permitted for copy >0");
    return;
  }
  if (!StoreRecoveryRecord(pFile))
    perror("StoreRecoveryRecord() failed");
  PrintString(disp, "stored\n");
}

/* ************************************************************************
   Function: CrashTest
   Description:
*/
static void CrashTest(void)
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
  disp_event_t ev;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));
  OpenSmallTerminal(disp);

  PrintString(disp, "~version:~ %d.%d.%d\n", vermaj, vermin, ver_revision);
  PrintString(disp, "~build:~ [%d]\n", verbld);
  PrintString(disp, "~configuration file version:~ %d\n", vercfg);
  PrintString(disp, "~build date/time:~ %s; %s\n", datebld, timebld);
  PrintString(disp, "~definitions:~\n");
#ifdef _DEBUG
  PrintString(disp, "_DEBUG_\n");
#endif
#ifdef HEAP_DBG
  PrintString(disp, "HEAP_DBG=%d\n", HEAP_DBG);
#endif
#ifdef WIN32
  PrintString(disp, "WIN32\n");
#endif
#ifdef _MSC_VER
  PrintString(disp, "_MSC_VER=%d\n", _MSC_VER);
#endif
#ifdef MSDOS
  PrintString(disp, "MSDOS\n");
#endif
#ifdef DJGPP
  PrintString(disp, "DJGPP=%d, DJGPP_MINOR=%d\n", DJGPP, DJGPP_MINOR);
#endif
#ifdef LINUX
  PrintString(disp, "LINUX\n");
#endif
#ifdef UNIX
  PrintString(disp, "UNIX\n");
#endif

  PrintString(disp, "\ndiagnostic terminal:\n");

_main_menu:
  PrintString(disp, "~[F]~ile ~[H]~eap ~[P]~aram ~[K]~eys ~[R]~ecoveryStore ~[B]~ookmarks\n~[S]~earch"
    " ~[C]~ontainer Cr~[a]~sh ~[N]~curses\n");

_wait_key:
  do
  {
    disp_event_read(disp, &ev);
  }
  while (ev.t.code != EVENT_KEY);
  Key = ev.e.kbd.key;

  if (Key == KEY(0, kbEsc))
  {
    CloseSmallTerminal();
    disp_event_clear(&ev);
    ev.t.code = EVENT_USR;
    ev.t.user_msg_code = MSG_INVALIDATE_SCR;
    ContainerHandleEvent(&stRootContainer, &ev);
    return;
  }

  switch (ASC(Key))
  {
    case 'f':
    case 'F':
      DiagFile(disp);
      break;
    case 'k':
    case 'K':
      DiagKeyNames(disp);
      break;
    case 'p':
    case 'P':
      DiagParameters(disp);
      break;
    case 'h':
    case 'H':
      DiagHeap(disp);
      break;
    case 'r':
    case 'R':
      RecoveryRecord(disp);
      break;
    case 'B':
    case 'b':
      DumpBookmarks(disp);
      break;
    case 's':
    case 'S':
      DumpSearchReplacePattern(&stSearchContext, disp);
      break;
    case 'c':
    case 'C':
      DumpContainersTree(&stRootContainer, 0, disp);
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

