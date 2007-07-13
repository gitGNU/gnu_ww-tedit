/*

File: winclip.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th July, 1999
Descrition:
  Functions that store and get text from the Microsoft Windows clipboard.

*/

#ifdef _WIN32
// we need some functions for pointer validation. So tell global.h that
// we need WINDOWS.H
#  ifndef USE_WINDOWS
#    define USE_WINDOWS
#  endif
#endif

/* entire file ifdef-ed */
#ifdef DISP_WIN32_GUIEMU

#include "global.h"
#include "block.h"
#include "wlimits.h"
#include "disp.h"
#include "palette.h"
#include "menu.h"
#include "winclip.h"

static dispc_t *s_disp;

/* ************************************************************************
   Function: GetFromWindowsClipboard
   Description:
*/
void GetFromWindowsClipboard(TBlock **pBlock)
{
  HGLOBAL hglb;
  LPSTR lpstr;
  UINT nColumnFormat;
  UINT nFormat;
  WORD blockattr;

  ASSERT(pBlock != NULL);

  nColumnFormat = RegisterClipboardFormat("MSDEVColumnSelect");
  ASSERT(nColumnFormat != 0);

  if (*pBlock != NULL)
    DisposeABlock(pBlock);

  if (!IsClipboardFormatAvailable(CF_OEMTEXT))
    return;
  if (!OpenClipboard(NULL))
    return;

  hglb = GetClipboardData(CF_OEMTEXT);
  if (hglb != NULL)
  {
    lpstr = GlobalLock(hglb);
    if (lpstr != NULL)
    {
      /*
      Enumerate to see whether nColumnFotmat is present
      */
      nFormat = 0;
      blockattr = 0;
      while (nFormat = EnumClipboardFormats(nFormat))
      {
        if (nFormat == nColumnFormat)
        {
          blockattr |= COLUMN_BLOCK;
          break;
        }
      }

      *pBlock = MakeBlock(lpstr, blockattr);

      GlobalUnlock(hglb);
    }
  }

  CloseClipboard();
}

/* ************************************************************************
   Function: PutToWindowsClipboard
   Description:
*/
void PutToWindowsClipboard(TBlock *pBlock)
{
  int nSize;
  HGLOBAL hglbCopy;
  LPSTR lpstrCopy;
  char *p;
  int i;
  UINT nColumnFormat;

  ASSERT(VALID_PBLOCK(pBlock));

  if (!OpenClipboard(NULL))
    return;
  EmptyClipboard();

  /*
  Calculate block size
  */
  nSize = 0;
  for (i = 0; i < pBlock->nNumberOfLines; ++i)
    nSize += pBlock->pIndex[i].nLen + 2;  /* +2 for trailing cr/lf marker */

  /*
  Allocate global memory to compose the clipboard text
  */
  hglbCopy = GlobalAlloc(GMEM_DDESHARE, nSize + 1);

  if (hglbCopy == NULL)
  {
    CloseClipboard();
    return;
  }

  /* Lock the handle and copy the text to the buffer. */
  lpstrCopy = GlobalLock(hglbCopy);
  p = lpstrCopy;

  for (i = 0; i < pBlock->nNumberOfLines; ++i)
  {
    strcpy(p, pBlock->pIndex[i].pLine);
    p = strchr(p, '\0');
    ASSERT(p != NULL);
    if (i != pBlock->nNumberOfLines - 1)
    {
      *p++ = '\r';
      *p++ = '\n';
    }
  }
  *p = '\0';

  GlobalUnlock(hglbCopy);
  /* Place the handle on the clipboard. */
  SetClipboardData(CF_OEMTEXT, hglbCopy);

  if (pBlock->blockattr & COLUMN_BLOCK)
  {
    nColumnFormat = RegisterClipboardFormat("MSDEVColumnSelect");
    SetClipboardData(nColumnFormat, NULL);
  }

  CloseClipboard();
}

static char *ClipHist[MAX_CLIP_HIST];
static CRITICAL_SECTION Section_ClipHist;

/* ************************************************************************
   Function: SelectClipboardHistory
   Description:
     Selects an element from the clipboard history.
*/
BOOLEAN SelectClipboardHistory(TBlock **ppBlock)
{
  int i;
  char *s;
  char *d;
  char *d2;
  TMenuItem stMenuItems[MAX_CLIP_HIST];
  TMenuItem *stItems[MAX_CLIP_HIST];
  char Items[MAX_CLIP_HIST][MAX_CLIP_HIST_WIN_WIDTH + 5];
  TMenu stMenu;
  int x;
  int y;
  int nSize;
  HGLOBAL hglbCopy;
  LPSTR lpstrCopy;
  UINT nColumnFormat;

  memset(stMenuItems, 0, sizeof(stMenuItems));
  memset(&stMenu, 0, sizeof(stMenu));
  memset(stItems, 0, sizeof(stItems));

  i = 0;

  while (i < MAX_CLIP_HIST)
  {
    s = ClipHist[i];
    if (s == NULL)
      break;
    ++s;  /* skip the block type byte */
    d = d2 = Items[i];
    sprintf(d, "~%d~ ", i + 1);
    d = strchr(d, '\0');  /* d points the end of the string */
    while (1)
    {
      if (*s == '\0')
        break;
      if (d - d2 > MAX_CLIP_HIST_WIN_WIDTH)
        break;
      if (*s == '\n')
      {
        *d++ = ' ';
        ++s;
        continue;
      }
      if (*s == ' ')
      {
        /* replace multiple spaces with only one space */
        if (*(d - 1) == ' ')
        {
          ++s;
          continue;
        }
      }
      if (*s < ' ')
      {
        ++s;
        continue;
      }
     *d++ = *s++;
    }
    *d = '\0';
    stMenuItems[i].Prompt = d2;
    stMenuItems[i].Command = i + 1;
    stItems[i] = &stMenuItems[i];
    ++i;
  }

  if (i == 0)
    return FALSE;

  stMenu.ItemsNumber = i;
  stMenu.Items = (TMenuItem *(*)[])stItems;
  stMenu.nPalette = coMenu;
  disp_cursor_get_xy(0, &x, &y);
  stMenu.disp = s_disp;
  i = Menu(&stMenu, x, y, 0, 0);
  if (i <= 0)
    return FALSE;

  --i;

  /*
  Put the selected item in the system clipboard
  */
  if (!OpenClipboard(NULL))
    return FALSE;
  EmptyClipboard();

  /*
  Calculate block size
  */
  nSize = strlen(ClipHist[i] + 1);

  /*
  Allocate global memory to compose the clipboard text
  */
  hglbCopy = GlobalAlloc(GMEM_DDESHARE, nSize + 1);

  if (hglbCopy == NULL)
  {
    CloseClipboard();
    return FALSE;
  }

  /* Lock the handle and copy the text to the buffer. */
  lpstrCopy = GlobalLock(hglbCopy);
  strcpy(lpstrCopy, ClipHist[i] + 1);

  GlobalUnlock(hglbCopy);
  /* Place the handle on the clipboard. */
  SetClipboardData(CF_OEMTEXT, hglbCopy);

  if (*ClipHist[i])
  {
    nColumnFormat = RegisterClipboardFormat("MSDEVColumnSelect");
    SetClipboardData(nColumnFormat, NULL);
  }

  CloseClipboard();

  return TRUE;
}

/* ************************************************************************
   Function: PutInHistory
   Description:
     Called whenever clipboard changes contents.
     Keeps a copy of the clipboard in the history array.
*/
static void PutInHistory(void)
{
  char *lpstr;
  UINT nFormat;
  int blockattr;
  HANDLE hglb;
  char *sBlock;
  UINT nColumnFormat;

  nColumnFormat = RegisterClipboardFormat("MSDEVColumnSelect");
  ASSERT(nColumnFormat != 0);

  if (!IsClipboardFormatAvailable(CF_OEMTEXT))
    return;
  if (!OpenClipboard(NULL))
    return;

  hglb = GetClipboardData(CF_OEMTEXT);
  if (hglb == NULL)
  {
_close_exit:
    CloseClipboard();
    return;
  }

  lpstr = GlobalLock(hglb);
  if (lpstr == NULL)
    goto _close_exit;

  /*
  Enumerate to see whether nColumnFotmat is present
  */
  nFormat = 0;
  blockattr = 0;
  while (nFormat = EnumClipboardFormats(nFormat))
  {
    if (nFormat == nColumnFormat)
    {
      blockattr |= COLUMN_BLOCK;
      break;
    }
  }
  sBlock = VirtualAlloc(NULL, strlen(lpstr) + 2, MEM_COMMIT, PAGE_READWRITE);
  if (sBlock == NULL)
    goto _close_exit;
  sBlock[0] = blockattr;
  strcpy(&sBlock[1], lpstr);

  EnterCriticalSection(&Section_ClipHist);
  /* insert the new block at position 0 in the history
  and move all the rest one position right */
  if (ClipHist[MAX_CLIP_HIST - 1] != NULL)
    VirtualFree(ClipHist[MAX_CLIP_HIST - 1], 0, MEM_RELEASE);
  memcpy(&ClipHist[1], &ClipHist[0], (MAX_CLIP_HIST - 1) * sizeof(char *));
  ClipHist[0] = sBlock;
  LeaveCriticalSection(&Section_ClipHist);

  GlobalUnlock(hglb);
  CloseClipboard();
}

BOOLEAN bQuit;

/* ************************************************************************
   Function: WindowProc
   Description:
     This is the hidden window messages procedure.
     The window context is only used as a mean of processing
     messages send to the console application.
*/
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static HWND hwndNextViewer;

  switch (uMsg)
  {
    case WM_CREATE:
      /* Add the window to the clipboard viewer chain */
      hwndNextViewer = SetClipboardViewer(hwnd);
      break;

    case WM_CHANGECBCHAIN:
      /* If the next window is closing, repair the chain */
      if ((HWND) wParam == hwndNextViewer)
          hwndNextViewer = (HWND) lParam;
      else /* Otherwise, pass the message to the next link */
        if (hwndNextViewer != NULL)
          SendMessage(hwndNextViewer, uMsg, wParam, lParam);
      break;

    case WM_DESTROY:
      ChangeClipboardChain(hwnd, hwndNextViewer);
      PostQuitMessage(0);
      bQuit = TRUE;
      break;

    case WM_DRAWCLIPBOARD:  /* clipboard contents changed */
      PutInHistory();
      /*
      Pass the message to the next window in clipboard
      viewer chain.
      */
      if (hwndNextViewer != 0)  /* BoundsChecker complained ;-) */
        SendMessage(hwndNextViewer, uMsg, wParam, lParam);
      break;

    default:
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }

  return 0;
}

HANDLE hEventStop;
HANDLE hThread;

/* ************************************************************************
   Function: MonitorThread
   Description:
     Creates an invisible window/thread that receives the
     clipboard update notifications.
*/
static DWORD WINAPI MonitorThread(LPVOID lpParameter)
{
  WNDCLASSEX wcx;
  HWND hwnd;
  MSG msg;
  HANDLE hObjects[1];

  wcx.cbSize = sizeof(wcx);            /* size of structure  */
  wcx.style = CS_HREDRAW | CS_VREDRAW; /* redraw if size changes */
  wcx.lpfnWndProc = MainWndProc;       /* points to window procedure */
  wcx.cbClsExtra = 0;                  /* no extra class memory */
  wcx.cbWndExtra = 0;                  /* no extra window memory */
  wcx.hInstance = 0;                   /* handle to instance */
  wcx.hIcon =
    LoadIcon(NULL, IDI_APPLICATION);   /* predefined app. icon */
  wcx.hCursor =
    LoadCursor(NULL, IDC_ARROW);       /* predefined arrow */
  wcx.hbrBackground =
    GetStockObject(WHITE_BRUSH);       /* white background brush */
  wcx.lpszMenuName = "MainMenu";       /* name of menu resource */
  wcx.lpszClassName = "MainWClass";    /* name of window class */
  wcx.hIconSm = 0;                     /* small class icon */

  if (RegisterClassEx(&wcx) == 0)
    return 0;

  hwnd = CreateWindow(
    "MainWClass",                      /* name of window class */
    "Sample",                          /* title-bar string */
     WS_OVERLAPPEDWINDOW,              /* top-level window */
     CW_USEDEFAULT,                    /* default horizontal position */
     CW_USEDEFAULT,                    /* default vertical position */
     CW_USEDEFAULT,                    /* default width */
     CW_USEDEFAULT,                    /* default height */
     (HWND) NULL,                      /* no owner window */
     (HMENU) NULL,                     /* use class menu */
     0,                                /* handle to application instance */
     (LPVOID) NULL);                   /* no window-creation data */

  if (hwnd == 0)
      return 0;

  hObjects[0] = hEventStop;
  bQuit = FALSE;

  while (!bQuit)
  {
    switch (MsgWaitForMultipleObjects(1, hObjects, FALSE, INFINITE, QS_ALLINPUT))
    {
      case WAIT_OBJECT_0:
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        bQuit = TRUE;
        break;
    }
    while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (bQuit)  /* but allow processing of the last message WM_CLOSE */
        break;
    }
  }

  return 0;
}

/* ************************************************************************
   Function: InstallWinClipboardMonitor
   Description:
     Creates an invisible window/thread that receives the
     clipboard update notifications.
*/
void InstallWinClipboardMonitor(dispc_t *disp)
{
  s_disp = disp;
  hEventStop = CreateEvent(0, FALSE, FALSE, 0);
  InitializeCriticalSection(&Section_ClipHist);
  hThread = CreateThread(0, 0, MonitorThread, 0, 0, 0);
}

/* ************************************************************************
   Function: UninstallWinClipboardMonitor
   Description:
     Notifies the monitor thread to quit
*/
void UninstallWinClipboardMonitor(void)
{
  SetEvent(hEventStop);
  WaitForSingleObject(hThread, 2000);
  CloseHandle(hEventStop);
  CloseHandle(hThread);
  DeleteCriticalSection(&Section_ClipHist);
}
#endif /* DISP_WIN32_GUIEMU */

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

