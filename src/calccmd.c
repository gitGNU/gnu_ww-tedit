/*

File: calccmd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 18th January, 2000
Descrition:
  Calculator.
  Uses calculator module written by Tzvetan Mikov <mikov@usa.net>

*/

#include "global.h"
#include "enterln.h"
#include "disp.h"
#include "wrkspace.h"
#include "defs.h"
#include "l2disp.h"
#include "calccmd.h"
#include "fpcalc/calc.h"
#include "cmdc.h"

/* ************************************************************************
   Function: CmdToolCalculator
   Description:
*/
void CmdToolCalculator(void *pCtx)
{
  static char sCalcBuf[MAX_CALC_BUF] = ""; // Ceco: added explicit init
  static char sDestBuf[MAX_CALC_BUF];
  disp_event_t ev;
  dispc_t *disp;

  disp = wrkspace_get_disp(CMDC_WRKSPACE(pCtx));

  while (EnterLn(sPromptCalculator, sCalcBuf,
         MAX_CALC_BUF, pCalculatorHistory, NULL, NULL, NULL, FALSE, disp))
  {
    if (Calc(sDestBuf, sCalcBuf) == 0)
    {
      // Success: let the user edit the result
      strcpy(sCalcBuf, sDestBuf);
    }
    else
    {
      // Error: show a message
      ConsoleMessageProc(disp, NULL, MSG_ERROR, NULL, sDestBuf);
    }
  }

  disp_event_clear(&ev);
  ev.t.code = EVENT_USR;
  ev.t.user_msg_code = MSG_INVALIDATE_SCR;
  ContainerHandleEvent(&stRootContainer, &ev);
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

