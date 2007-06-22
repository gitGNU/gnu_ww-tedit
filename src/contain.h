/*

File: contain.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 1st July, 2002
Descrition:
  Events, Views, Splitters.
  Views and splitters container.

*/

#ifndef CONTAIN_H
#define CONTAIN_H

#include "disp.h"

/* TView.pfnHandleEvent(nEvent) */
#define KBD_MOUSE_EVENT 1
#define EVENT_PAINT 2

/* event.t.user_msg_code */
enum msg_codes
{
  MSG_UPDATE_SCR = 1,
  MSG_UPDATE_STATUS_LN,
  MSG_INVALIDATE_SCR,
  MSG_SET_FOCUS,
  MSG_KILL_FOCUS,
  MSG_EXECUTE_COMMAND,
  MSG_SET_MIN_SIZE,
  MSG_RELEASE_MOUSE,
  MSG_NOTIFY_CLOSE
};

typedef struct _View
{
  #ifdef _DEBUG
  BYTE MagicByte;
  #define VIEW_MAGIC  0x59
  #endif

  /* bActive would be FALSE for fileviews that are not
  currently on the screen */
  BOOLEAN bOnFocus;  /* View contains the current keyboard focus */
  BOOLEAN bDockedView;  /* Always inserted at the bottom of the screen */

  int x;
  int y;
  int nWidth;
  int nHeight;
  int (*pfnHandleEvent)(disp_event_t *pEvent, struct _View *pView);

  /* Used to identify a view in a restored session */
  /* Only file views are reopened!                 */
  char sViewID[_MAX_PATH];  /* points to the file title (pFile->sTitle) */

  void *pViewReserved;  /* View specific data */

  void *pContainer;  /* Holding container of NULL if view is not visible */
} TView;

typedef struct _Container
{
  #ifdef _DEBUG
  BYTE MagicByte;
  #define CONTAINER_MAGIC  0x58
  #endif

  BOOLEAN bTaken;

  int x;
  int y;
  int nWidth;
  int nHeight;

  int nMinWidth;
  int nMinHeight;

  /* We have either a view (pView != NULL)
  either subcointainers (pSub1 != NULL && pSub2 != NULL) */

  TView *pView;

  struct _Container *pSub1;
  struct _Container *pSub2;
  struct _Container *pParent;
  int nProportion;
  BOOLEAN bHorizontal;

  int nSplitter;  /* splitter index in the array of splitters */
  dispc_t *disp;
  struct wrkspace_data *wrkspace;
} TContainer;

/* To be used in ASSERT()! */
#ifdef _DEBUG
#define VALID_PCONTAINER(pCont) (pCont != NULL && pCont->MagicByte == CONTAINER_MAGIC)
#else
#define VALID_PCONTAINER(pCont) (1)
#endif

/* To be used in ASSERT()! */
#ifdef _DEBUG
#define VALID_PVIEW(pView) (pView != NULL && pView->MagicByte == VIEW_MAGIC)
#else
#define VALID_PVIEW(pView) (1)
#endif

void ViewInit(TView *pView);
void ContainerInit(TContainer *pCont, TContainer *pParent, int x, int y,
  int nWidth, int nHeight, dispc_t *disp, struct wrkspace_data *wrkspace);
void ContainerSetFocus(TContainer *pRoot, TContainer *pCont);
void ContainerSetView(TContainer *pCont, TView *pView);
int ContainerHandleEvent(TContainer *pCont, disp_event_t *pEvent);
TContainer *ContainerSplit(TContainer *pCont, TView *pNewView, int nProportion,
  int bHorizontal);
void ContainerRemoveView(TContainer *pCont);
int ContainerWalkTree(TContainer *pCont,
  int (*pfnAction)(TContainer *pCont, void *pCtx), void *pCtx);
void ContainerCollapse(TContainer *pCont);

#endif  /* CONTAIN_H */

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

