/*

File: kbd.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Keyboard manipulation functions.

*/

#ifndef _KBD_H
#define _KBD_H

/*
Shift state mask constants
*/
#define kbShift      1
#define kbReserved   2
#define kbCtrl       4
#define kbAlt        8
#define kbScroll     16
#define kbNum        32
#define kbCaps       64

/*
Convertion macroses.
A key is universaly presented by a DWORD -- hi WORD is shift state, lo
WORD is KEY code; KEY code on other hand is as follows -- hi BYTE is
keyboard scan code for this key, lo BYTE is ascii code of the key.
*/
#define KEY(state, scan)	(((DWORD)(((DWORD)state) << 16) | (scan << 8)))
#define ASC(key)	((char)(key & 0x000000ff))
#define SH_STATE(key)	((WORD)((key & 0xffff0000l) >> 16))
#define SCANCODE(key)	((BYTE)((key & 0x0000ff00) >> 8))
#define NO_ASC(key)     ((DWORD)(key & 0xffffff00l))
#define NO_SH_STATE(key)	((DWORD)(key & 0x0000ffff))
#define NO_SH_STATE2(state, key)    ((DWORD)(key & ~KEY(state, 0)))

extern BOOLEAN bCtrlReleased;  /* Indicates whether <CTRL> has released */
extern WORD ShiftState;  /* Current Alt, Ctrl and Shifts state */
extern int nTimeElapsed;  /* Local session time elapsed in seconds */

/*
nGlobalH:M:S can be used as a clock, the idea here in 'W' is
in the w.ini file to be stored elapsed time while 'W' is working
When 'W' is started nGlobalH:M:S is loaded with the values from w.ini,
on exit in W.INI is stored current nGlobalH:M:S.
*/
extern int nGlobalHours;  /* Global hours elapsed */
extern int nGlobalMinutes;  /* Global minutes elapsed */
extern int nGlobalSeconds;  /* Global seconds elapsed */

#if 0
enum event_type2
{
  EVENT_NONE = -1,
  EVENT_KEY = 1,
  EVENT_MOUSE,
  EVENT_RESIZE,
  EVENT_RECOVERY_TIMER_EXPIRED,
  EVENT_CLIPBOARD_PASTE,
  EVENT_CLIPBOARD_CLEAR,
  EVENT_CLIPBOARD_COPY_REQUESTED,
  MSG_UPDATE_SCR,
  MSG_UPDATE_STATUS_LN,
  MSG_INVALIDATE_SCR,
  MSG_SET_FOCUS,
  MSG_KILL_FOCUS,
  MSG_EXECUTE_COMMAND,
  MSG_SET_MIN_SIZE,
  MSG_RELEASE_MOUSE,
  MSG_NOTIFY_CLOSE
};
#endif

#define MOUSE_BUTTON1 1
#define MOUSE_RIGHTMOST 2
#define MOUSE_BUTTON2 4
#define MOUSE_BUTTON3 8
#define MOUSE_BUTTON4 16

#define MOUSE_DOUBLE_CLICK 1
#define MOUSE_MOVE 2
#define MOUSE_WHEEL 4

#if 0
disp_event_t
{
  union
  {
    enum event_type code;
    int msg;
  } t;

  union
  {
    int nParam;
    DWORD nKey;
    char *pdata;

    struct
    {
      int param1;
      int param2;
      int param3;
      int param4;
      int param5;
    } p;

    struct
    {
      int NewX1;
      int NewY1;
      int NewWidth;
      int NewHeight;
    } stNewSizeEvent;

    struct
    {
      int x;
      int y;
      int nButtonState;
      int nCtrlState;
      int nFlags;
      int nButtonData;
    } stMouseEvent;
  } e;
};
#endif

BOOLEAN kbhit(int *pnSleepTime);
void ReadEvent(disp_event_t *pEvent);
void GetKeyName(DWORD dwKey, char *psKeyName);
DWORD ReadKey(void);

extern const char *KeyNames[];  /* Key names definitions -- used by GetKeyName() */

#endif /* ifndef _KBD_H */

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

