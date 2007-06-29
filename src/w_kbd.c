/*

File: w_kbd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 6th October, 1998
Descrition:
  Keyboard manipulation functions.

*/

#include "global.h"

#ifndef _NON_TEXT  /* text mode console only */

#include "options.h"
#include "kbd.h"
#include "scr.h"
#include "keydefs.h"

BOOLEAN bCtrlReleased = TRUE;
WORD ShiftState;
extern HANDLE hInput;
extern HANDLE hTimer;
extern HANDLE w_objs[2];

int nTimeElapsed = 0;
int nGlobalHours = 0;  /* Global hours elapsed */
int nGlobalMinutes = 0;  /* Global minutes elapsed */
int nGlobalSeconds = 0;  /* Global seconds elapsed */

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

/* ************************************************************************
   Function: TranslateWKey
   Description:
     Translates from EVENT_RECORD to DWORD keycode
*/
static BOOLEAN TranslateWKey(KEY_EVENT_RECORD *k, DWORD *code)
{
  char AsciiCode;
  WORD ScanCode;

  if (!k->bKeyDown)
  {
    if (k->wVirtualScanCode == 0x1d)
      bCtrlReleased = TRUE;
    return (FALSE);
  }

  if (k->wVirtualScanCode == 0x1d ||  /* Ctrl pressed */
    k->wVirtualScanCode == 0x38 ||  /* Alt pressed */
    k->wVirtualScanCode == 0x2a ||  /* LShift pressed */
    k->wVirtualScanCode == 0x36 ||  /* RShift pressed */
    k->wVirtualScanCode == 0x3a ||  /* CapsLock pressed */
    k->wVirtualScanCode == 0x46 ||  /* ScrollLock pressed */
    k->wVirtualScanCode == 0x45)  /* NumLock pressed */
    return (FALSE);

  ShiftState = 0;
  AsciiCode = k->uChar.AsciiChar;
  ScanCode = k->wVirtualScanCode;

  if (AsciiCode < 32)  /* Ctrl combination */
    AsciiCode = 0;

  if (k->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
  {
    ShiftState |= kbAlt;
    AsciiCode = 0;
  }

  if (k->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
    ShiftState |= kbCtrl;

  if (k->dwControlKeyState & SHIFT_PRESSED)
    ShiftState |= kbShift;

  /* Check for NumLock and num-pad keys */
  if (AsciiCode != 0)
  {
    if (ScanCode == kbPrSc || ScanCode == kbHome ||
       ScanCode == kbUp || ScanCode == kbPgUp ||
       ScanCode == kbGrayMinus || ScanCode == kbLeft ||
       ScanCode == kbPad5 || ScanCode == kbRight ||
       ScanCode == kbGrayPlus || ScanCode == kbEnd ||
       ScanCode == kbDown || ScanCode == kbPgDn ||
       ScanCode == kbIns || ScanCode == kbDel)
    ScanCode = 0;
  }

  *code = ((DWORD)ShiftState) << 16 | ScanCode << 8 | AsciiCode;

  return (TRUE);
}

/* ************************************************************************
   Function: ReadEvent
   Description:
     Waits for an event from the console.
   When returns 0xffff nRecoveryTime expired.
*/
void ReadEvent(struct event *pEvent)
{
  INPUT_RECORD k;
  DWORD n;
  DWORD code;
  BOOLEAN f;

  f = TRUE;
  memset(pEvent, 0, sizeof(struct event));

  do
  {
    pEvent->t.code = EVENT_NONE;
    if (WaitForSingleObject(hInput, 1000) == WAIT_TIMEOUT)
    {
      /* Increase the global time elapsed */
      ++nGlobalSeconds;
      if (nGlobalSeconds >= 60)
      {
	++nGlobalMinutes;
	nGlobalSeconds = 0;
	if (nGlobalMinutes >= 60)
	{
	  ++nGlobalHours;
	  nGlobalMinutes = 0;
	}
      }

      if (nRecoveryTime > 0 && ++nTimeElapsed % nRecoveryTime == 0)
      {
        pEvent->t.code = EVENT_RECOVERY_TIMER_EXPIRED;
        return;
      }
      continue;
    }

    if (!ReadConsoleInput(hInput, &k, 1, &n))
      ASSERT(0);  /* What is the problem here?! */

    switch (k.EventType)
    {
      case KEY_EVENT:
        if (TranslateWKey(&k.Event.KeyEvent, &code))
        {
          f = FALSE;
          pEvent->t.code = EVENT_KEY;
          pEvent->e.nKey = code;
        }
        break;

      case MOUSE_EVENT:
        pEvent->t.code = EVENT_MOUSE;
        pEvent->e.stMouseEvent.x =
          k.Event.MouseEvent.dwMousePosition.X + nLeft;
        pEvent->e.stMouseEvent.y =
          k.Event.MouseEvent.dwMousePosition.Y + nTop;

        pEvent->e.stMouseEvent.nButtonState = 0;
        if (k.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
          pEvent->e.stMouseEvent.nButtonState |= MOUSE_BUTTON1;
        if (k.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED)
          pEvent->e.stMouseEvent.nButtonState |= MOUSE_RIGHTMOST;
        if (k.Event.MouseEvent.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED)
          pEvent->e.stMouseEvent.nButtonState |= MOUSE_BUTTON2;
        if (k.Event.MouseEvent.dwButtonState & FROM_LEFT_3RD_BUTTON_PRESSED)
          pEvent->e.stMouseEvent.nButtonState |= MOUSE_BUTTON3;
        if (k.Event.MouseEvent.dwButtonState & FROM_LEFT_4TH_BUTTON_PRESSED)
          pEvent->e.stMouseEvent.nButtonState |= MOUSE_BUTTON4;

        pEvent->e.stMouseEvent.nCtrlState = 0;
        if (k.Event.MouseEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
          pEvent->e.stMouseEvent.nCtrlState |= kbAlt;
        if (k.Event.MouseEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
          pEvent->e.stMouseEvent.nCtrlState |= kbCtrl;
        if (k.Event.MouseEvent.dwControlKeyState & SHIFT_PRESSED)
          pEvent->e.stMouseEvent.nCtrlState |= kbShift;

        pEvent->e.stMouseEvent.nFlags = 0;
        switch (k.Event.MouseEvent.dwEventFlags)
        {
          case DOUBLE_CLICK:
            pEvent->e.stMouseEvent.nFlags = MOUSE_DOUBLE_CLICK;
            break;
          case MOUSE_MOVED:
            pEvent->e.stMouseEvent.nFlags = MOUSE_MOVE;
            break;
          case MOUSE_WHEELED:
            pEvent->e.stMouseEvent.nFlags = MOUSE_WHEEL;
            switch (k.Event.MouseEvent.dwButtonState)
            {
              case 0x780000:
                pEvent->e.stMouseEvent.nButtonData = 1;
                break;
              case 0xff880000:
                pEvent->e.stMouseEvent.nButtonData = 2;
                break;
              case 0x780004:
                pEvent->e.stMouseEvent.nButtonData = 3;
                break;
              case 0xff880004:
                pEvent->e.stMouseEvent.nButtonData = 4;
                break;
            }
            break;
        }
        f = FALSE;
        break;

      case WINDOW_BUFFER_SIZE_EVENT:
        pEvent->t.code = EVENT_RESIZE;
        pEvent->e.stNewSizeEvent.NewWidth =
          k.Event.WindowBufferSizeEvent.dwSize.X;
        pEvent->e.stNewSizeEvent.NewHeight =
          k.Event.WindowBufferSizeEvent.dwSize.Y;
        f = FALSE;
        break;
    }
  }
  while (f);

  return;
}

/* ************************************************************************
   Function: ReadKey
   Description:
     Filters all events out and returns only the key events.
*/
DWORD ReadKey(void)
{
  struct event Event;

  Event.t.code = EVENT_NONE;
  while (Event.t.code != EVENT_KEY && Event.t.code != EVENT_RECOVERY_TIMER_EXPIRED)
    ReadEvent(&Event);
  return Event.e.nKey;
}

#endif

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

