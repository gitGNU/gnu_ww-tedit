/*

File: wrkspace.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th November, 1998
Descrition:
  Organises work space file storage. This includes file-in-memory storage and
  MRU list storage.

*/

#ifndef WRKSPACE_H
#define WRKSPACE_H

#include "filenav.h"
#include "mru.h"
#include "history.h"
#include "block.h"
#include "bookm.h"
#include "search.h"
#include "ctxhelp.h"
#include "contain.h"

extern TFileList *pFilesInMemoryList;
extern TMRUList *pMRUFilesList;
extern THistory *pFileOpenHistory;
extern THistory *pCalculatorHistory;
extern THistory *pFindHistory;
extern THistory *pFindInFilesHistory;
extern TBlock *pClipboard;
extern TBookmarksSet stUserBookmarks;
extern TBookmarksSet stFindInFiles1;
extern TBookmarksSet stFindInFiles2;
extern TBookmarksSet stInfoHistory;
extern TBookmarksSet stOutput;  /* log files, tools output, etc. */
extern TSearchContext stSearchContext;
extern TContextHelp stCtxHelp;
extern TContainer stRootContainer;
extern TContainer *pModalContainer;  /* oveloads stRootContainer if not NULL */
extern TContainer *pCurrentContainer;
extern TContainer *pMessagesContainer;
extern BOOLEAN b_clipbrd_owner;

struct wrkspace_data;
typedef struct wrkspace_data wrkspace_data_t;

int  wrkspace_obj_size(void);
int  wrkspace_init(void *wrkspace_obj);
void wrkspace_done(wrkspace_data_t *wrkspace);

void     wrkspace_set_disp(wrkspace_data_t *wrkspace, dispc_t *disp);
dispc_t *wrkspace_get_disp(wrkspace_data_t *wrkspace);

void wrkspace_get_shift_state(wrkspace_data_t *wrkspace,
                              int *shift_state, int *ctrl_is_released);
void wrkspace_store_shift_state(wrkspace_data_t *wrkspace,
                                int shift_state, int ctrl_is_released);

void InitWorkspace(wrkspace_data_t *wrkspace);
TFile *GetCurrentFile(void);
int GetNumberOfFiles(void);
void DoneWorkspace(void);

/* To be used in ASSERT()! */
#ifdef _DEBUG
int wrkspace_is_valid(const wrkspace_data_t *wrkspace);
#define VALID_WRKSPACE(wrkspace) (wrkspace_is_valid(wrkspace))
#else
#define VALID_WRKSPACE(wrkspace) (1)
#endif

#endif  /* WRKSPACE_H */

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

