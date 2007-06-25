/*

File: xclip.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th Jun, 2005
Descrition:
  Functions that store and get text from the X Window clipboard.

*/

#include "global.h"

#include "xclip.h"
#include "xscr_i.h"
#include <X11/Xatom.h>

/* ************************************************************************
   Function: xclipbrd_request
   Description:
     This function will ask selection owner to send an event
     "SelectionNotify" to our window
*/
void xclipbrd_request(struct disp_ctx *pdisp_ctx, int b_get_primary)
{
  Window win_sel_owner;
  Atom selection_type;
  Atom prop;

  selection_type = b_get_primary ? XA_PRIMARY : XA_SECONDARY;
  win_sel_owner = XGetSelectionOwner(pdisp_ctx->x11_disp, selection_type);
  prop = XInternAtom(pdisp_ctx->x11_disp, "WW_SELECTION", FALSE);
  if (win_sel_owner != None)
  {
    /*TRACE1("XConvertSelection into prop atom %x\n", prop);*/
    XConvertSelection(pdisp_ctx->x11_disp,
      selection_type,
      XA_STRING,
      prop,
      pdisp_ctx->h_win,
      CurrentTime);
    XFlush(pdisp_ctx->x11_disp);
  }
}

/* ************************************************************************
   Function: xclipbrd_free
   Description:
     When clipboard data is no longer needed this function must be
     called to dispose the memory buffer
*/
void xclipbrd_free(char *sel_data)
{
  XFree(sel_data);
}

/* ************************************************************************
   Function: xclipbrd_announce
   Description:
     This announces that this application has produced content for
     the clipboard
*/
void xclipbrd_announce(struct disp_ctx *pdisp_ctx, int b_primary)
{
  XSetSelectionOwner(pdisp_ctx->x11_disp,
    b_primary ? XA_PRIMARY : XA_SECONDARY,
    pdisp_ctx->h_win, CurrentTime);
  XFlush(pdisp_ctx->x11_disp);
}

/* ************************************************************************
   Function: xclipbrd_send
   Description:
     If any other application requests the clipboard data, then
     xclipbrd_send() must be used to dispatch the data toward the
     requestor
   TODO:
     if pdata is NULL, 
*/
void xclipbrd_send(struct disp_ctx *pdisp_ctx,
  int requestor, int requestor_property, int time, int target_type,
  const char *pdata, int b_primary)
{
  XEvent clipbrd_event;

  /*TRACE3("XChangeProperty: requestor=%x, requestor_property=%x, target=%x\n",
    requestor, requestor_property, XA_STRING);*/
  XChangeProperty(pdisp_ctx->x11_disp, requestor, requestor_property,
    XA_STRING, 8, PropModeReplace, pdata, strlen(pdata) + 1);

  clipbrd_event.xselection.type = SelectionNotify;
  clipbrd_event.xselection.serial = 0;
  clipbrd_event.xselection.send_event = True;
  clipbrd_event.xselection.requestor = requestor;
  clipbrd_event.xselection.selection = b_primary ? XA_PRIMARY : XA_SECONDARY;
  clipbrd_event.xselection.target = target_type;
  clipbrd_event.xselection.property = requestor_property;
  clipbrd_event.xselection.time = time;  /* can't be CurrentTime! */
  clipbrd_event.xselection.display = pdisp_ctx->x11_disp;
  XSendEvent(pdisp_ctx->x11_disp, requestor, 0, 0, &clipbrd_event);
  XFlush(pdisp_ctx->x11_disp);
}

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

