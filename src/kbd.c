/*

File: kbd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 27th October, 1998
Descrition:
  Keyboard manipulation functions.
  DOS

*/

#include "global.h"
#include "options.h"
#include "kbd.h"

#include <bios.h>
#include <dpmi.h>

BOOLEAN bCtrlReleased = TRUE;
WORD ShiftState;
static int nTimer = 0;  /* Mark last second in 50ms units -- BIOS timer */
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

  ScanCode = (dwKey & 0x0000ff00L) >> 8;
  AsciiCode = (dwKey & 0x000000ffL);
  ShiftState = (dwKey & 0xffff0000L) >> 16;

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
   Function: TranslateKey
   Description:
     Translates bioskey toward universal dword key combination code
     DWORD consist of (WORD)ShiftStates,(BYTE)ScanCode,(BYTE)AsciiCode
*/
DWORD TranslateKey(WORD dwKey, int ShiftState)
{
  BYTE ScanCode;
  BYTE AsciiCode;

  ScanCode = dwKey >> 8;
  AsciiCode = dwKey & 0x00ff;

  if (ScanCode != 0 && ((dwKey & 0x00ff) == 0xe0))
    dwKey &= 0xff00;  /* Make enhanced key to be like normal */

  if (AsciiCode < 32)  /* Control combination? */
    if (ScanCode != 0)  /* If not Alt+Num combinatio */
      AsciiCode = 0;  /* Will be available only as combination */

  if (AsciiCode == 0xe0)
    AsciiCode = 0;  /* Enhanced key -- clean the fake ascii code */

  if (ScanCode == 0x85)
    ScanCode = 87;  /* F11 */

  if (ScanCode == 0x86)
    ScanCode = 88;  /* F12 */

  if (ShiftState & kbShift)
  {
    if (ScanCode >= 84 && ScanCode <= 93)  /* Shift F1-F10 */
      ScanCode -= (84 - 59);
    else
      if (ScanCode == 0x87)
        ScanCode = 87;
      else
        if (ScanCode == 0x88)
	  ScanCode = 88;
	else
	  if (AsciiCode >= '0' && AsciiCode <= '9')  /* Shift+NumPad */
	    AsciiCode = 0;
	  else
	    if (AsciiCode == '.')  /* Shift+Del */
	      AsciiCode = 0;
  }

  if (ShiftState & kbAlt)
  {
    if (ScanCode >= 104 && ScanCode <= 113)  /* F1 - F10 */
      ScanCode -= (104 - 59);
    else
      if (ScanCode >= 120 && ScanCode <= 131)  /* 1 - = */
        ScanCode -= (120 - 2);
      else
        if (ScanCode == 0x8b)  /* F11 */
	  ScanCode = 87;
	else
	  if (ScanCode == 0x8c)  /* F12 */
	    ScanCode = 88;
  }

  if (ShiftState & kbCtrl)
  {
    switch (ScanCode)
    {
      case 0x92:  /* Ins */
        ScanCode = 0x52;
        break;
      case 0x93:  /* Del */
        ScanCode = 0x53;
        break;
      case 0x94:  /* Tab */
        ScanCode = 15;
        break;
      case 0x77:  /* Home */
        ScanCode = 0x47;
        break;
      case 0x8d:  /* Up */
        ScanCode = 0x48;
        break;
      case 0x84:  /* PgUp */
        ScanCode = 0x49;
        break;
      case 0x8e:  /* Gray - */
        ScanCode = 0x4a;
        break;
      case 0x73:  /* Left */
        ScanCode = 0x4b;
	break;
      case 0x8f:  /* Pad5 */
        ScanCode = 0x4c;
        break;
      case 0x74:  /* Right */
        ScanCode = 0x4d;
        break;
      case 0x90:  /* Gray + */
        ScanCode = 0x4e;
        break;
      case 0x75:  /* End */
        ScanCode = 0x4f;
        break;
      case 0x91:  /* Down */
        ScanCode = 0x50;
        break;
      case 0x76:  /* PgDn */
        ScanCode = 0x51;
        break;
      case 0x89:  /* F11 */
	ScanCode = 87;
	break;
      case 0x8a:  /* F12 */
        ScanCode = 88;
	break;
      default:
        if (ScanCode >= 94 && ScanCode <= 103)
          ScanCode -= (94 - 59);
    }
  }

  if (ScanCode == 0)  /* Alt+numpad combination */
    ShiftState = 0;  /* This may be kbAlt state */

  return ((((DWORD)ShiftState) << 16) | (ScanCode << 8) | AsciiCode);
}

/* ************************************************************************
   Function: ReadKey
   Description:
     Waits for a key
*/
DWORD ReadKey(void)
{
  WORD dwKey;

  while (!bioskey(0x11))
  {
    __dpmi_yield();  /* Reduce the processor usage while under Windows */
    if ((biostime(_TIME_GETCLOCK, 0) - nTimer) > 19)
    {
      /*
      1 second elapsed. Check for nRecoveryTime
      */
      nTimer = biostime(_TIME_GETCLOCK, 0);

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
	return (0xffff);
      else
	continue;
    }
    /* Check whether Ctrl key is released */
    if ((bioskey(2) & kbCtrl) == 0)
      bCtrlReleased = TRUE;
  }

  ShiftState = bioskey(2);
  if (ShiftState & 2)  /* RightShift? */
  {
    ShiftState &= ~2;  /* Clear this flag */
    ShiftState |= 1;  /* Raise the Shift flag */
  }

  ShiftState &= 0xf;  /* Indicate only Shift, Alt and Ctrl */

  dwKey = (WORD)bioskey(0x10);

  return (TranslateKey(dwKey, ShiftState));
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

