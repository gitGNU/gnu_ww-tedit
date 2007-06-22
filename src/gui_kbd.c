/*

File: gui_kbd.c
Project: W, Layer2 -- WIN32 GUI application
Started: 19th August, 2003
Descrition:
  Win32 GUI application screen API functions.

*/

#include "global.h"

#ifdef _NON_TEXT  /* win32 gui only */

#include "kbd.h"
#include "scr.h"

BOOLEAN bCtrlReleased = TRUE;
WORD ShiftState;
extern HANDLE hInput;
extern HANDLE hTimer;
extern HANDLE w_objs[2];

int nTimeElapsed = 0;
int nGlobalHours = 0;  /* Global hours elapsed */
int nGlobalMinutes = 0;  /* Global minutes elapsed */
int nGlobalSeconds = 0;  /* Global seconds elapsed */

extern HWND g_hWnd;
extern SIZE g_ChSize;
extern POINT RealCaretPos;
extern BOOLEAN bCaretVisible;
extern BOOLEAN bOwnCaret;
extern void OnPaint ( HDC hdc, const RECT * upd );

/* ************************************************************************
   Function: GetKeyName
   Description:
     Produces a key name combination by a particular code
   On exit:
     KeyName - key combination image string
*/
void GetKeyName(DWORD dwKey, char *psKeyName)
{
  BYTE ScanCode;
  BYTE AsciiCode;
  WORD ShiftState;

  ScanCode = (BYTE)((dwKey & 0x0000ff00L) >> 8);
  AsciiCode = (BYTE)(dwKey & 0x000000ffL);
  ShiftState = (WORD)((dwKey & 0xffff0000L) >> 16);

  psKeyName[0] = '\0';

  if (ScanCode == 0)  /* Code from alt+numpad combination */
  {
    sprintf(psKeyName, "ASCII: %d", AsciiCode);
    return;
  }

  if (ShiftState & kbCtrl)
    strcat(psKeyName, "Ctrl+");

  if (ShiftState & kbAlt)
    strcat(psKeyName, "Alt+");

  if (ShiftState & kbShift)
    strcat(psKeyName, "Shift+");

  if (ScanCode > 83)
  {
    if (ScanCode == 87)
      strcat(psKeyName, "F11");
    else
      if (ScanCode == 88)
        strcat(psKeyName, "F12");
      else
        sprintf(strchr(psKeyName, '\0'), "<%d>", ScanCode);
  }
  else
    strcat(psKeyName, KeyNames[ScanCode - 1]);
}

#define EVENT_QUEUE_SIZE 32
static unsigned EvH, EvT, EvC;
static disp_event_t EvQueue[EVENT_QUEUE_SIZE];

//--------------------------------------------------------------------------
// Name         EvQInit
//
//
//--------------------------------------------------------------------------
void EvQInit ( void )
{
  EvC = EvH = EvT = 0;
}

//--------------------------------------------------------------------------
// Name         QueueEvent
//
//
//--------------------------------------------------------------------------
static void QueueEvent ( const disp_event_t *pEv )
{
  if (EvC < EVENT_QUEUE_SIZE)
  {
    memcpy(&EvQueue[EvT], pEv, sizeof(disp_event_t));
    EvT = (EvT + 1) % EVENT_QUEUE_SIZE;
    ++EvC;
  }
}

//--------------------------------------------------------------------------
// Name         UnqueueEvent
//
//
//--------------------------------------------------------------------------
static void UnqueueEvent ( disp_event_t *pEv )
{
  if (EvC == 0)
    return;

  memcpy(pEv, &EvQueue[EvH], sizeof(disp_event_t));
  EvH = (EvH + 1) % EVENT_QUEUE_SIZE;
  --EvC;
}

//--------------------------------------------------------------------------
// Name         QueueKey
//
//
//--------------------------------------------------------------------------
static void QueueKey ( DWORD key )
{
  disp_event_t stEvent;

  stEvent.t.code = EVENT_KEY;
  stEvent.e.nKey = key;
  QueueEvent(&stEvent);
}

#if 0
#define KEY_TRACE0 TRACE0
#define KEY_TRACE1 TRACE1
#define KEY_TRACE2 TRACE2
#else
#define KEY_TRACE0 (void)
#define KEY_TRACE1 (void)
#define KEY_TRACE2 (void)
#endif

//--------------------------------------------------------------------------
// Name         ProcessKey
//
// Transitions:
//    WM_KEYDOWN -> WM_CHAR -> WM_KEYUP (produces ascii and ctrl characters)
//    WM_SYSKEYDOWN -> WM_SYSCHAR -> WM_SYSKEYUP (produces all Alt combinations)
//    WM_KEYDOWN -> WM_KEYUP (F1-F10, arrows, Home, End, etc.)
//
//--------------------------------------------------------------------------
static void ProcessKey(unsigned message, unsigned param1, unsigned param2)
{
  BYTE AsciiCode;
  BYTE ScanCode;
  WORD ShiftState;
  DWORD code;
  MSG msg;
  char buf[1024];

  switch (message)
  {
    case WM_KEYUP:
      KEY_TRACE0("WM_KEYUP\n");
      break;
    case WM_SYSKEYUP:
      KEY_TRACE0("WM_SYSKEUP\n");
      break;
    case WM_CHAR:
      KEY_TRACE0("WM_CHAR ");
      break;
    case WM_DEADCHAR:
      KEY_TRACE0("WM_DEADCHAR ");
      break;
    case WM_SYSCHAR:
      KEY_TRACE0("WM_SYSCHAR ");
      break;
    case WM_SYSDEADCHAR:
      KEY_TRACE0("WM_SYSDEADCHAR ");
      break;
    case WM_KEYDOWN:
      KEY_TRACE0("WM_KEYDOWN ");
      break;
    case WM_SYSKEYDOWN:
      KEY_TRACE0("WM_SYSKEYDOWN ");
      break;
    default:
      KEY_TRACE0("WM_UKNOWN ");
  }

  AsciiCode = param1 & 255;
  ScanCode = (BYTE)((param2 >> 16) & 255);
  ShiftState = 0;
  if (GetKeyState( VK_MENU ) < 0)
    ShiftState |= kbAlt;
  if (GetKeyState( VK_CONTROL ) < 0)
    ShiftState |= kbCtrl;
  if (GetKeyState( VK_SHIFT ) < 0)
    ShiftState |= kbShift;
  code = ((DWORD)ShiftState) << 16 | (ScanCode << 8) | AsciiCode;

  if (message == WM_KEYUP && ScanCode == kb_Ctrl)
    bCtrlReleased = TRUE;

  if (message == WM_SYSKEYDOWN)
  {
    if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
    {
      if (msg.message == WM_SYSCHAR)
      {
        KEY_TRACE0("[WM_SYSCHAR] ");
        AsciiCode = 0;
        ScanCode = (BYTE)((msg.lParam >> 16) & 255);
        code = ((DWORD)ShiftState) << 16 | (ScanCode << 8) | AsciiCode;
        GetKeyName(code, buf);
        KEY_TRACE2("sys_key: %s, ascii: %c\n", buf, AsciiCode);
        QueueKey(code);
        PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
      }
    }
    else
      if (!(ScanCode == kb_Ctrl || ScanCode == kb_RShift ||
          ScanCode == kb_LShift || ScanCode == kb_Alt ||
          ScanCode == kbCapsLock || ScanCode == kbNumLock ||
          ScanCode == kbScrLock))
      {
        AsciiCode = 0;
        code = ((DWORD)ShiftState) << 16 | (ScanCode << 8) | AsciiCode;
        GetKeyName(code, buf);
        KEY_TRACE2("sys_key: %s, ascii: %c\n", buf, AsciiCode);
        QueueKey(code);
      }
  }
  else if (message == WM_KEYDOWN)
  {
    if (!(ScanCode == kb_Ctrl || ScanCode == kb_RShift ||
        ScanCode == kb_LShift || ScanCode == kb_Alt ||
        ScanCode == kbCapsLock || ScanCode == kbNumLock ||
        ScanCode == kbScrLock))
    {
      if (ScanCode == kbPrSc || ScanCode == kbHome ||
         ScanCode == kbUp || ScanCode == kbPgUp ||
         ScanCode == kbGrayMinus || ScanCode == kbLeft ||
         ScanCode == kbPad5 || ScanCode == kbRight ||
         ScanCode == kbGrayPlus || ScanCode == kbEnd ||
         ScanCode == kbDown || ScanCode == kbPgDn ||
         ScanCode == kbIns || ScanCode == kbDel)
      ScanCode = 0;  /* in case of num-lock */

      if (ScanCode == kbEsc)
        AsciiCode = 0;
      if (ScanCode == kbEnter)
        AsciiCode = 0;
      if (ScanCode == kbTab)
        AsciiCode = 0;
      if (ScanCode == kbBckSpc)
        AsciiCode = 0;

      if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE) && msg.message == WM_CHAR)
      {
        KEY_TRACE0("[WM_CHAR] ");
        if (AsciiCode != 0)
          AsciiCode = msg.wParam & 255;
        code = ((DWORD)ShiftState) << 16 | (ScanCode << 8) | AsciiCode;
        GetKeyName(code, buf);
        KEY_TRACE2("key: %s, ascii: %c\n", buf, AsciiCode);
        QueueKey(code);
        PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
      }
      else
      {
        ScanCode = (BYTE)((param2 >> 16) & 255);
        AsciiCode = 0;  /* a non WM_CHAR key */
        code = ((DWORD)ShiftState) << 16 | (ScanCode << 8) | AsciiCode;
        GetKeyName(code, buf);
        KEY_TRACE1("just_a_key2: %s\n", buf);
        QueueKey(code);
      }
    }
  }
}

static int align_size(int new_size, int units)
{
  new_size /= units;
  new_size *= units;
  return new_size;
}

//--------------------------------------------------------------------------
// Name         WndProc
//
//
//--------------------------------------------------------------------------
LRESULT CALLBACK WndProc (
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam
    )
{
  WINDOWPOS *win_pos;
  WINDOWPLACEMENT wndpl;
  extern int caption_height;
  extern int border_width;
  static int last_width = 0;
  static int last_height = 0;
  static int last_x = 0;
  static int last_y = 0;
  int resize_x;
  int resize_y;
  int move_x;
  int move_y;
  int new_height;
  int new_width;
  disp_event_t stEvent;

  if (!(GetKeyState( VK_CONTROL ) < 0))
    bCtrlReleased = TRUE;

  switch (message)
  {
    case WM_SETFOCUS:
      CreateCaret(
        g_hWnd,
        (HBITMAP)NULL,
        max( 2, GetSystemMetrics( SM_CXBORDER ) ),
        g_ChSize.cy
      );
      SetCaretPos( RealCaretPos.x, RealCaretPos.y );
      if (bCaretVisible)
        ShowCaret( g_hWnd );
      bOwnCaret = TRUE;
      break;

    case WM_KILLFOCUS:
      DestroyCaret();
      bOwnCaret = FALSE;
      break;

    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        HDC hdc;

        hdc = BeginPaint( hWnd, &ps );
        OnPaint( hdc, &ps.rcPaint );
        EndPaint( hWnd, &ps );
      }
      break;

    case WM_MOVE:
      if (GetWindowPlacement(g_hWnd, &wndpl))
      {
        ScreenPosX = wndpl.rcNormalPosition.left;
        ScreenPosY = wndpl.rcNormalPosition.top;
      }
      break;

    case WM_WINDOWPOSCHANGING:
      win_pos = (WINDOWPOS *)lParam;
      if (win_pos->flags & SWP_NOSIZE)  /* no new size in this event? */
        return 0;

      if (IsIconic(hWnd))  /* event is _minimize_? */
        return 0;  /* not an interesting event */

      /*
      {cx,cy} must only hold client area for the computations
      */
      win_pos->cx -= border_width * 2;
      win_pos->cy -= caption_height + border_width;

      /*
      Current cx/cy is _last_ width/height next time
      */
      if (last_width == 0)
        last_width = win_pos->cx;
      if (last_height == 0)
        last_height = win_pos->cy;
      if (last_x == 0)
        last_x = win_pos->x;
      if (last_y == 0)
        last_y = win_pos->y;
      resize_x = win_pos->cx - last_width;
      resize_y = win_pos->cy - last_height;
      move_x = win_pos->x - last_x;
      move_y = win_pos->y - last_y;
      last_width = win_pos->cx;
      last_height = win_pos->cy;
      last_x = win_pos->x;
      last_y = win_pos->y;

      /*
      Compute (align) the new size
      */
      if (IsZoomed(hWnd))  /* event is _maximize_? */
      {
        bMaximized = TRUE;
      }
      else
      {
        if (bMaximized)  /* restoring from maximized? */
          bMaximized = FALSE;  /* Old size must be fine */
        else
        {
          /*
          {cx, cy} width and height could only be set
          to character size aligned positions
          */
          win_pos->cx = align_size(win_pos->cx, g_ChSize.cx);
          if (move_x != 0 && resize_x != 0)  /* resize left side? */
            win_pos->x += (last_width - win_pos->cx);
          win_pos->cy = align_size(win_pos->cy, g_ChSize.cy);
          if (move_y != 0 && resize_y != 0)  /* resize top side? */
            win_pos->y += (last_height - win_pos->cy);
        }
      }

      /*
      Adjust back to include border and caption area
      */
      new_width = win_pos->cx / g_ChSize.cx;
      new_height = win_pos->cy / g_ChSize.cy;
      win_pos->cx += border_width * 2;
      win_pos->cy += caption_height + border_width;

      /*
      Generate resize event if there is new size
      */
      if (new_height != ScreenHeight || new_width != ScreenWidth)
      {
        stEvent.t.code = EVENT_RESIZE;
        stEvent.e.stNewSizeEvent.NewX1 = 0;
        stEvent.e.stNewSizeEvent.NewY1 = 0;
        if (bMaximized)
        {
          scr_width_before_maximize = ScreenWidth;
          scr_height_before_maximize = ScreenHeight;
        }
        ScreenHeight = new_height;
        ScreenWidth = new_width;
        CurHeight = ScreenHeight - 1;
        CurWidth = ScreenWidth - 1;
        stEvent.e.stNewSizeEvent.NewWidth = ScreenWidth;
        stEvent.e.stNewSizeEvent.NewHeight = ScreenHeight;
        Scr_ReallocBuf();

        if (disp_handle_resize != NULL)
          disp_handle_resize(&stEvent);
        else
          QueueEvent(&stEvent);
      }
      break;

    case WM_CHAR:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_SYSCHAR:
    case WM_SYSKEYUP:
    case WM_KEYUP:
    case WM_DEADCHAR:
    case WM_SYSDEADCHAR:
      ProcessKey(message, (unsigned)wParam, (unsigned)lParam);
      break;

    case WM_CLOSE:
      break;

    case WM_DESTROY:
      PostQuitMessage( 0 );
      break;

    case WM_NCDESTROY:
      g_hWnd = NULL;
      // fallthrough

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }

  return 0;
};

/* ************************************************************************
   Function: ReadEvent
   Description:
     Waits for an event from the console.
   When returns 0xffff nRecoveryTime expired.
*/
void ReadEvent(disp_event_t *pEvent)
{
  MSG msg;

  pEvent->t.code = EVENT_NONE;
  for(;;)
  {
    if (EvC > 0)
    {
      UnqueueEvent(pEvent);
      return;
    }

    if (!GetMessage( &msg, NULL, 0, 0 ))
      exit( 0 );
    TranslateMessage( &msg );
    DispatchMessage( &msg );
  }
}

/* ************************************************************************
   Function: ReadKey
   Description:
     Filters all events out and returns only the key events.
*/
DWORD ReadKey(void)
{
  disp_event_t Event;

  Event.t.code = EVENT_NONE;
  while (Event.t.code != EVENT_KEY && Event.t.code != EVENT_RECOVERY_TIMER_EXPIRED)
    ReadEvent(&Event);
  return Event.e.nKey;
}

#endif  /* _NON_TEXT only */

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2005
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
